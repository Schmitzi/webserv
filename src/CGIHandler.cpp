#include "../include/CGIHandler.hpp"
#include "../include/Client.hpp"

CGIHandler::CGIHandler() {
    _input[0] = -1;
    _input[1] = -1;
    _output[0] = -1;
    _output[1] = -1;
}

CGIHandler::~CGIHandler() {
    cleanupResources();
}

void CGIHandler::setClient(Client &client) {
    _client = &client;
}

void CGIHandler::setServer(Server &server) {
    _server = &server;
}

std::string CGIHandler::getInfoPath() {
    return _pathInfo;
}

int CGIHandler::executeCGI(Client &client, Request &req, std::string const &scriptPath) {
    cleanupResources();
    _path = scriptPath;
   
    if (doChecks(client, req) == 1)
        return 1;
    if (prepareEnv(req) == 1)
        return 1;

    // Convert environment to char* array
    std::vector<char*> env_ptrs;
    for (size_t i = 0; i < _env.size(); i++) {
        env_ptrs.push_back(const_cast<char*>(_env[i].c_str()));
    }
    env_ptrs.push_back(NULL);

    // Get interpreter and build arguments based on config
    std::string interpreter = getInterpreterFromConfig(req, _path);
    std::vector<char*> args_ptrs;
    
    if (!interpreter.empty()) {
        // For interpreted scripts (PHP, Python, Perl, etc.)
        args_ptrs.push_back(const_cast<char*>(interpreter.c_str()));
        args_ptrs.push_back(const_cast<char*>(_path.c_str()));
    } else {
        // For executable scripts (compiled CGI programs, shell scripts)
        args_ptrs.push_back(const_cast<char*>(_path.c_str()));
    }
    args_ptrs.push_back(NULL);

    std::cout << getTimeStamp(_client->getFd()) << BLUE << "Executing CGI: " << RESET;
    if (!interpreter.empty()) {
        std::cout << interpreter << " " << _path << std::endl;
    } else {
        std::cout << _path << std::endl;
    }

    pid_t pid = fork();
    
    if (pid == 0) {  // Child process
        close(_input[1]);  // Close write end
        close(_output[0]); // Close read end
        
        // Redirect stdin and stdout
        if (dup2(_input[0], STDIN_FILENO) < 0 || 
            dup2(_output[1], STDOUT_FILENO) < 0) {
            std::cerr << "dup2 failed" << std::endl;
            exit(1);
        }
        
        close(_input[0]);
        close(_output[1]);
        
        // Change to script directory for relative paths
        std::string scriptDir = getDirectoryFromPath(_path);
        if (!scriptDir.empty()) {
            chdir(scriptDir.c_str());
        }
        
        // Execute the script
        if (!interpreter.empty()) {
            execve(interpreter.c_str(), args_ptrs.data(), env_ptrs.data());
        } else {
            execve(_path.c_str(), args_ptrs.data(), env_ptrs.data());
        }
        
        std::cerr << "execve failed: " << strerror(errno) << std::endl;
        exit(1);
    } 
    else if (pid > 0) {  // Parent process
        close(_input[0]);  // Close read end
        close(_output[1]); // Close write end

        // Write request body to script's stdin (for POST requests)
        if (!req.getBody().empty()) {
            ssize_t written = write(_input[1], req.getBody().c_str(), req.getBody().length());
            if (!checkReturn(_client->getFd(), written, "write()", "Failed to write to CGI stdin")) {
                close(_input[1]);
                _input[1] = -1;
                client.sendErrorResponse(500, req);
                cleanupResources();
                return 1;
            }
            std::cout << getTimeStamp(_client->getFd()) << BLUE << "Wrote " << written 
                      << " bytes to CGI stdin" << RESET << std::endl;
        }
        close(_input[1]);
        _input[1] = -1;

        // Wait for script to complete with timeout
        int status;
        if (waitForCGICompletion(pid, 30) != 0) {
            std::cerr << getTimeStamp(_client->getFd()) << RED << "CGI timeout" << RESET << std::endl;
            client.sendErrorResponse(504, req);
            cleanupResources();
            return 1;
        }
        
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            std::cout << getTimeStamp(_client->getFd()) << GREEN << "CGI completed successfully" << RESET << std::endl;
            int result = processScriptOutput(client);
            cleanupResources();
            return result;
        } else {
            std::cerr << getTimeStamp(_client->getFd()) << RED << "CGI failed with exit code: " 
                      << WEXITSTATUS(status) << RESET << std::endl;
            client.sendErrorResponse(500, req);
            cleanupResources();
            return 1;
        }
    }
    
    client.sendErrorResponse(500, req);
    cleanupResources();
    return 1;
}

// Get interpreter from config based on file extension
std::string CGIHandler::getInterpreterFromConfig(Request& req, const std::string& scriptPath) {
    size_t dotPos = scriptPath.find_last_of('.');
    if (dotPos == std::string::npos) {
        return ""; // No extension, assume executable
    }
    
    std::string extension = scriptPath.substr(dotPos); // Include the dot
    std::string pattern = "~ \\" + extension + "$"; // e.g., "~ \.php$"
    
    // Find matching location in config
    std::map<std::string, locationLevel>::iterator it = req.getConf().locations.begin();
    for (; it != req.getConf().locations.end(); ++it) {
        if (it->first == pattern) {
            return it->second.cgiProcessorPath;
        }
    }
    
    // Fallback: try without regex pattern (just the extension)
    for (it = req.getConf().locations.begin(); it != req.getConf().locations.end(); ++it) {
        if (it->first.find(extension) != std::string::npos) {
            return it->second.cgiProcessorPath;
        }
    }
    
    return ""; // No interpreter found, assume executable
}

// Helper to extract directory from full path
std::string CGIHandler::getDirectoryFromPath(const std::string& fullPath) {
    size_t lastSlash = fullPath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        return fullPath.substr(0, lastSlash);
    }
    return "";
}

// Wait for CGI completion with timeout
int CGIHandler::waitForCGICompletion(pid_t pid, int timeoutSeconds) {
    time_t startTime = time(NULL);
    
    while (true) {
        int status;
        pid_t result = waitpid(pid, &status, WNOHANG);
        
        if (result == pid) {
            return 0; // Process completed
        } else if (result == -1) {
            return 1; // Error
        }
        
        if (time(NULL) - startTime > timeoutSeconds) {
            std::cerr << getTimeStamp(_client->getFd()) << RED << "CGI timeout, killing process " 
                      << pid << RESET << std::endl;
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            return 1; // Timeout
        }
        
        usleep(100000); // Sleep 100ms
    }
}

int CGIHandler::processScriptOutput(Client &client) {
    std::string output;
    char buffer[4096];
    int totalBytesRead = 0;
    
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(_output[0], &readfds);
    
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    
    int selectResult = select(_output[0] + 1, &readfds, NULL, NULL, &timeout);
    
    if (selectResult > 0 && FD_ISSET(_output[0], &readfds)) {
        ssize_t bytesRead;
        while ((bytesRead = read(_output[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;
            totalBytesRead += bytesRead;
            std::cout << getTimeStamp(_client->getFd()) << BLUE << "Received " << bytesRead 
                      << " bytes from CGI" << RESET << std::endl;
        }
        if (bytesRead < 0) {
            std::cerr << getTimeStamp(client.getFd()) << RED << "Error reading CGI output" << RESET << std::endl;
            close(_output[0]);
            _output[0] = -1;
            return 1;
        }
    }
    
    close(_output[0]);
    _output[0] = -1;
    std::cout << getTimeStamp(_client->getFd()) << BLUE << "Total CGI output: " << totalBytesRead 
              << " bytes" << RESET << std::endl;

    if (output.empty()) {
        std::string defaultResponse = "HTTP/1.1 200 OK\r\n";
        defaultResponse += "Content-Type: text/plain\r\n";
        defaultResponse += "Content-Length: 22\r\n\r\n";
        defaultResponse += "No output from script\n";
        
        ssize_t sent = send(client.getFd(), defaultResponse.c_str(), defaultResponse.length(), 0);
        if (!checkReturn(_client->getFd(), sent, "send()", "Failed to send default response"))
            return 1;
        return 0;
    }

    std::pair<std::string, std::string> headerAndBody = splitHeaderAndBody(output);
    std::string headerSection = headerAndBody.first;
    std::string bodyContent = headerAndBody.second;
    
    std::map<std::string, std::string> headerMap = parseHeaders(headerSection);
    
    if (isChunkedTransfer(headerMap))
        return handleChunkedOutput(headerMap, bodyContent);
    else
        return handleStandardOutput(headerMap, bodyContent);
}

bool CGIHandler::isChunkedTransfer(const std::map<std::string, std::string>& headers) {
    std::map<std::string, std::string>::const_iterator it = headers.find("Transfer-Encoding");
    if (it != headers.end() && it->second.find("chunked") != std::string::npos)
        return true;
    return false;
}

int CGIHandler::handleStandardOutput(const std::map<std::string, std::string>& headerMap, const std::string& initialBody) {
    // Build complete HTTP response
    std::string response = "HTTP/1.1 200 OK\r\n";
    
    // Add CGI headers
    for (std::map<std::string, std::string>::const_iterator it = headerMap.begin(); 
         it != headerMap.end(); ++it) {
        response += it->first + ": " + it->second + "\r\n";
    }
    
    // Add server headers if not already present
    if (headerMap.find("Content-Type") == headerMap.end()) {
        response += "Content-Type: text/html\r\n";
    }
    if (headerMap.find("Content-Length") == headerMap.end()) {
        response += "Content-Length: " + tostring(initialBody.length()) + "\r\n";
    }
    
    response += "Server: WebServ/1.0\r\n";
    response += "Connection: keep-alive\r\n";
    response += "\r\n";
    response += initialBody;
    
    if (!_client->sendAll(_client->getFd(), response)) {
        std::cerr << getTimeStamp(_client->getFd()) << RED << "Failed to send CGI response" << RESET << std::endl;
        return 1;
    }
    
    std::cout << getTimeStamp(_client->getFd()) << GREEN << "Sent CGI response (" 
              << response.length() << " bytes)" << RESET << std::endl;
    return 0;
}

int CGIHandler::handleChunkedOutput(const std::map<std::string, std::string>& headerMap, const std::string& initialBody) {
    std::string response = "HTTP/1.1 200 OK\r\n";
    
    for (std::map<std::string, std::string>::const_iterator it = headerMap.begin(); 
         it != headerMap.end(); ++it) {
        if (it->first != "Content-Length") {
            response += it->first + ": " + it->second + "\r\n";
        }
    }
    
    if (headerMap.find("Transfer-Encoding") == headerMap.end()) {
        response += "Transfer-Encoding: chunked\r\n";
    }
    
    response += "Connection: keep-alive\r\n";
    response += "\r\n";
    response += formatChunkedResponse(initialBody);
    
    if (!_client->sendAll(_client->getFd(), response)) {
        std::cerr << getTimeStamp(_client->getFd()) << RED << "Failed to send chunked CGI response" << RESET << std::endl;
        return 1;
    }
    
    return 0;
}

std::string CGIHandler::formatChunkedResponse(const std::string& body) {
    std::string chunkedBody = "";
    
    if (body.empty()) {
        chunkedBody = "0\r\n\r\n";
        return chunkedBody;
    }
    
    const size_t chunkSize = 4096;
    size_t remaining = body.length();
    size_t offset = 0;
    
    while (remaining > 0) {
        size_t currentChunkSize = (remaining < chunkSize) ? remaining : chunkSize;
        
        std::stringstream hexStream;
        hexStream << std::hex << currentChunkSize;
        chunkedBody += hexStream.str() + "\r\n";
        
        chunkedBody += body.substr(offset, currentChunkSize) + "\r\n";
        
        offset += currentChunkSize;
        remaining -= currentChunkSize;
    }
    
    chunkedBody += "0\r\n\r\n";
    return chunkedBody;
}

std::pair<std::string, std::string> CGIHandler::splitHeaderAndBody(const std::string& output) {
    size_t headerEnd = output.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = output.find("\n\n");
        if (headerEnd == std::string::npos) {
            return std::make_pair("", output);
        } else {
            return std::make_pair(
                output.substr(0, headerEnd),
                output.substr(headerEnd + 2)
            );
        }
    } else {
        return std::make_pair(
            output.substr(0, headerEnd),
            output.substr(headerEnd + 4)
        );
    }
}

std::map<std::string, std::string> CGIHandler::parseHeaders(const std::string& headerSection) {
    std::map<std::string, std::string> headers;
    std::istringstream iss(headerSection);
    std::string line;
    
    while (std::getline(iss, line) && !line.empty() && line != "\r") {
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }
        
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            headers[key] = value;
        }
    }
    
    return headers;
}

bool CGIHandler::isCGIScript(const std::string& path) {
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        return false; // No extension
    }
    
    std::string extension = path.substr(dotPos); // Include the dot
    
    // Check if any location in the current config handles this extension
    if (_server && !_server->getConfigs().empty()) {
        const serverLevel& config = _server->getConfigs()[0];
        std::map<std::string, locationLevel>::const_iterator it = config.locations.begin();
        
        for (; it != config.locations.end(); ++it) {
            // Check for regex pattern like "~ \.php$"
            if (it->first.find(extension) != std::string::npos && 
                !it->second.cgiProcessorPath.empty()) {
                return true;
            }
        }
    }
    
    return false;
}

void CGIHandler::doQueryStuff(const std::string text, std::string& fileName, std::string& fileContent) {
    if (!text.empty()) {
        std::istringstream ss(text);
        std::string pair;

        while (std::getline(ss, pair, '&')) {
            size_t pos = pair.find('=');
            if (pos == std::string::npos)
                continue;
            std::string key = pair.substr(0, pos);
            std::string value = pair.substr(pos + 1);

            if (key == "file" || key == "name" || key == "test")
                fileName = value;
            else if (key == "content" || key == "body" || key == "data" || key == "text" || key == "value")
                fileContent = value;
            else {
                fileContent = value;
            }
        }
    }
}

int CGIHandler::prepareEnv(Request &req) {
    _env.clear();
    
    // Required CGI environment variables
    _env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    _env.push_back("SERVER_SOFTWARE=WebServ/1.0");
    
    // Server information
    if (!req.getConf().servName.empty() && !req.getConf().servName[0].empty()) {
        _env.push_back("SERVER_NAME=" + req.getConf().servName[0]);
    } else {
        _env.push_back("SERVER_NAME=localhost");
    }
    
    _env.push_back("SERVER_PORT=" + tostring(_server->getConfParser().getPort(req.getConf())));
    _env.push_back("SERVER_PROTOCOL=HTTP/1.1");
    
    // Request information
    _env.push_back("REQUEST_METHOD=" + req.getMethod());
    _env.push_back("QUERY_STRING=" + req.getQuery());
    _env.push_back("CONTENT_TYPE=" + req.getContentType());
    _env.push_back("CONTENT_LENGTH=" + tostring(req.getBody().length()));
    
    // Script information
    _env.push_back("SCRIPT_NAME=" + req.getPath()); // The URL path to the script
    _env.push_back("SCRIPT_FILENAME=" + _path);     // Full filesystem path to script
    
    // PATH_INFO handling (for URLs like /script.php/extra/path)
    std::string pathInfo = extractPathInfo(req.getPath(), _path);
    _env.push_back("PATH_INFO=" + pathInfo);
    
    // Add HTTP headers as environment variables
    std::map<std::string, std::string> headers = req.getHeaders();
    for (std::map<std::string, std::string>::iterator it = headers.begin(); 
         it != headers.end(); ++it) {
        std::string envName = "HTTP_" + it->first;
        // Convert to uppercase and replace - with _
        std::transform(envName.begin(), envName.end(), envName.begin(), ::toupper);
        std::replace(envName.begin(), envName.end(), '-', '_');
        _env.push_back(envName + "=" + it->second);
    }
    
    // Add some common environment variables
    _env.push_back("REDIRECT_STATUS=200");
    
    return 0;
}

// Extract PATH_INFO from the request path
std::string CGIHandler::extractPathInfo(const std::string& requestPath, const std::string& scriptPath) {
    // Find the script name in the request path
    size_t scriptNamePos = requestPath.find_last_of('/');
    if (scriptNamePos == std::string::npos) {
        return "";
    }
    
    std::string scriptName = requestPath.substr(scriptNamePos + 1);
    size_t extPos = scriptName.find_last_of('.');
    if (extPos != std::string::npos) {
        // Check if there's additional path info after the full script name
        size_t fullScriptEnd = scriptNamePos + 1 + scriptName.length();
        if (fullScriptEnd < requestPath.length()) {
            return requestPath.substr(fullScriptEnd);
        }
    }
    
    return "";
}

void CGIHandler::cleanupResources() {
    for (int i = 0; i < 2; i++) {
        if (_input[i] >= 0) {
            close(_input[i]);
            _input[i] = -1;
        }
        if (_output[i] >= 0) {
            close(_output[i]);
            _output[i] = -1;
        }
    }
    _path.clear();
    _args.clear();
    _env.clear();
}

int CGIHandler::doChecks(Client client, Request& req) {
    if (access(_path.c_str(), F_OK) != 0) {
        std::cerr << getTimeStamp(_client->getFd()) << RED << "Script does not exist: " << RESET << _path << std::endl;
        client.sendErrorResponse(404, req);
        return 1;
    }

    if (access(_path.c_str(), R_OK) != 0) {
        std::cerr << getTimeStamp(_client->getFd()) << RED << "Script is not readable: " << RESET << _path << std::endl;
        client.sendErrorResponse(403, req);
        return 1;
    }

    if (pipe(_input) < 0 || pipe(_output) < 0) {
        std::cerr << getTimeStamp(_client->getFd()) << RED << "Pipe creation failed" << RESET << std::endl;
        client.sendErrorResponse(500, req);
        return 1;
    }
    return 0;
}