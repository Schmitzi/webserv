#include "../include/CGIHandler.hpp"
#include "../include/Client.hpp"

CGIHandler::CGIHandler() : _args(NULL) {
	// Initialize arrays
    _input[0] = -1;
    _input[1] = -1;
    _output[0] = -1;
    _output[1] = -1;
}

CGIHandler::CGIHandler(Client& client) : _args(NULL) {
    // Initialize arrays
	_client = &client;
	_server = &_client->getServer();
	_config = _server->getCurConfig();
	setCGIBin(&_config);
}

CGIHandler::~CGIHandler() {
    cleanupResources();
}

void    CGIHandler::setClient(Client &client) {
    _client = &client;
}

void    CGIHandler::setServer(Server &server) {
    _server = &server;
}

void    CGIHandler::setConfig(serverLevel& config) {
    _config = config;
}

std::string CGIHandler::getInfoPath() {
    return _pathInfo;
}

void    CGIHandler::setCGIBin(serverLevel *config) {
    _cgiBinPath = ""; // Initialize
    
    std::map<std::string, locationLevel>::iterator it = config->locations.begin();
    for (; it != config->locations.end(); ++it) {
        if (it->first.find("cgi-bin") != std::string::npos) {
            _cgiBinPath = it->second.cgiProcessorPath;
            break;
        }
    }
    
    if (_cgiBinPath.empty()) {
        // std::cout << RED << "Warning: No PHP CGI processor found in config" << RESET << "\n";
        // std::cout << RED << "Setting CGI-Bin to /usr/bin/php-cgi\n" << RESET;
        _cgiBinPath = "/usr/bin/php-cgi";
    }
}

int CGIHandler::executeCGI(Client &client, Request &req, std::string const &scriptPath) {
    cleanupResources();
    _path = scriptPath;
   
    setPathInfo(req);
    if (doChecks(client, req) == 1) {
        return 1;
    }
    prepareEnv(req);

    pid_t pid = fork();
    
    if (pid == 0) {  // Child process
        close(_input[1]);
        close(_output[0]);
        
        if (dup2(_input[0], STDIN_FILENO) < 0 || 
            dup2(_output[1], STDOUT_FILENO) < 0) {
            std::cerr << "dup2 failed\n";
            // exit(1);
			return 1;
        }
        close(_input[0]);
        close(_output[1]);
        
        execve(_args[0], _args, &_env[0]);
        
        std::cerr << "execve failed: " << strerror(errno) << "\n";
        // exit(1);
		return 1;
    } 
    else if (pid > 0) {  // Parent process
        close(_input[0]);
        close(_output[1]);

        if (!req.getBody().empty()) {
            write(_input[1], req.getBody().c_str(), req.getBody().length());
        }
        close(_input[1]);
        _input[1] = -1;

        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            
            
            if (exit_status == 0) {
                std::cerr << GREEN << getTimeStamp() << "CGI Script exit status: " << RESET << exit_status << "\n";
                int result = processScriptOutput(client);
                cleanupResources();
                return result;
            } else {
                std::cerr << RED << getTimeStamp() << "CGI Script exit status: " << RESET << exit_status << "\n";
                client.sendErrorResponse(500, req);
                cleanupResources();
                return 1;
            }
        } else {
            client.sendErrorResponse(500, req);
            cleanupResources();
            return 1;
        }
    }
    
    client.sendErrorResponse(500, req);
    cleanupResources();
    return 1;
}

int CGIHandler::processScriptOutput(Client &client) {
    std::string output;
    char buffer[4096];
    int totalBytesRead = 0;
    
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(_output[0], &readfds);
    
    struct timeval timeout;
    timeout.tv_sec = 10;  // 10 seconds timeout
    timeout.tv_usec = 0;
    
    int selectResult = select(_output[0] + 1, &readfds, NULL, NULL, &timeout);
    
    if (selectResult > 0 && FD_ISSET(_output[0], &readfds)) {
        ssize_t bytesRead;
        while ((bytesRead = read(_output[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;
            totalBytesRead += bytesRead;
            std::cout << BLUE << getTimeStamp() << "Received bytes: " << RESET << bytesRead << BLUE << ", Total buffer: " << RESET << totalBytesRead << "\n";
        }
    }
    
    close(_output[0]);
    _output[0] = -1;

    std::cout << BLUE << getTimeStamp() << "Total bytes read: " << RESET << totalBytesRead << std::endl;

    // If no output, send a default response
    if (output.empty()) {
        std::string defaultResponse = "HTTP/1.1 200 OK\r\n";
        defaultResponse += "Content-Type: text/plain\r\n";
        defaultResponse += "Content-Length: 22\r\n\r\n";
        defaultResponse += "No output from script\n";
        
        send(client.getFd(), defaultResponse.c_str(), defaultResponse.length(), 0);
        return 0;
    }

    // Split headers and body
    std::pair<std::string, std::string> headerAndBody = splitHeaderAndBody(output);
    std::string headerSection = headerAndBody.first;
    std::string bodyContent = headerAndBody.second;
    
    // Parse headers
    std::map<std::string, std::string> headerMap = parseHeaders(headerSection);
    
    // Check if we need to handle chunked transfer encoding
    if (isChunkedTransfer(headerMap)) {
        return handleChunkedOutput(headerMap, bodyContent);
    } else {
        return handleStandardOutput(headerMap, bodyContent);
    }
}

bool CGIHandler::isChunkedTransfer(const std::map<std::string, std::string>& headers) {
    std::map<std::string, std::string>::const_iterator it = headers.find("Transfer-Encoding");
    if (it != headers.end() && it->second.find("chunked") != std::string::npos) {
        return true;
    }
    return false;
}

int CGIHandler::handleStandardOutput(const std::map<std::string, std::string>& headerMap, const std::string& initialBody) {
    // Create a temporary request to hold the response information
    Request temp;
    
    // Set content type from headers
    std::map<std::string, std::string>::const_iterator typeIt = headerMap.find("Content-Type");
    if (typeIt != headerMap.end()) {
        temp.setContentType(typeIt->second);
    } else {
        temp.setContentType("text/html");
    }
    
    // Set the body
    temp.setBody(initialBody);
    
    // Send standard response
    _client->sendResponse(temp, "keep-alive", temp.getBody());
    return 0;
}

int CGIHandler::handleChunkedOutput(const std::map<std::string, std::string>& headerMap, const std::string& initialBody) {
    // Build HTTP chunked response
    std::string response = "HTTP/1.1 200 OK\r\n";
    
    // Add all headers except Content-Length (which doesn't apply to chunked responses)
    for (std::map<std::string, std::string>::const_iterator it = headerMap.begin(); it != headerMap.end(); ++it) {
        if (it->first != "Content-Length") {
            response += it->first + ": " + it->second + "\r\n";
        }
    }
    
    // Make sure Transfer-Encoding: chunked is included
    if (headerMap.find("Transfer-Encoding") == headerMap.end()) {
        response += "Transfer-Encoding: chunked\r\n";
    }
    
    response += "Connection: keep-alive\r\n";
    response += "\r\n";
    
    // Format body as chunked
    response += formatChunkedResponse(initialBody);
    
    // Send the chunked response
    if (!_client->send_all(_client->getFd(), response)) {
        std::cerr << "Failed to send chunked response" << std::endl;
        return 1;
    }
    
    return 0;
}

std::string CGIHandler::formatChunkedResponse(const std::string& body) {
    std::string chunkedBody = "";
    
    // If body is empty, just send a terminating chunk
    if (body.empty()) {
        chunkedBody = "0\r\n\r\n";
        return chunkedBody;
    }
    
    // Define chunk size (we could optimize by using different sizes)
    const size_t chunkSize = 4096;
    size_t remaining = body.length();
    size_t offset = 0;
    
    // Break body into chunks
    while (remaining > 0) {
        size_t currentChunkSize = (remaining < chunkSize) ? remaining : chunkSize;
        
        // Add chunk header (size in hex)
        std::stringstream hexStream;
        hexStream << std::hex << currentChunkSize;
        chunkedBody += hexStream.str() + "\r\n";
        
        // Add chunk data
        chunkedBody += body.substr(offset, currentChunkSize) + "\r\n";
        
        offset += currentChunkSize;
        remaining -= currentChunkSize;
    }
    
    // Add terminating chunk
    chunkedBody += "0\r\n\r\n";
    
    return chunkedBody;
}

std::pair<std::string, std::string> CGIHandler::splitHeaderAndBody(const std::string& output) {
    size_t headerEnd = output.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = output.find("\n\n");
        if (headerEnd == std::string::npos) {
            // If no header/body separator found, assume it's all body
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
        // Remove trailing '\r' if present
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }
        
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            // Trim whitespace
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
    
    if (dotPos != std::string::npos) {
        std::string ext = path.substr(dotPos + 1);
        
        static const char* whiteList[] = {"py", "php", "cgi", "pl", NULL};
        
        for (int i = 0; whiteList[i] != NULL; ++i) {
            if (ext.find(whiteList[i]) != std::string::npos) {
                return true;
            }
        }
    }
    return false;
}

void CGIHandler::prepareEnv(Request &req) {
    std::string ext = "";
	std::string filePath = "";
	std::string queryType = "";
	std::string fileName = "";
	std::string fileContent = "";
	if (!req.getQuery().empty()) {
		size_t pos1 = req.getQuery().find_first_of('=');
		size_t pos2 = req.getQuery().find_last_of('=');
		if (pos1 != std::string::npos && pos2 != std::string::npos && pos1 == pos2) {
			queryType = req.getQuery().substr(0, pos1);
			if (queryType == "file")
				fileName = req.getQuery().substr(pos1 + 1);
			else if (queryType == "content" || queryType == "body" || queryType == "data"
					|| queryType == "value" || queryType == "text" || queryType == "user")
				fileContent = req.getQuery().substr(pos1 + 1);
			else {
				std::cerr << "Invalid query type: " << queryType << std::endl;//TODO: send error response? is this even valid?
				return;
			}
		}
		locationLevel loc;
		if (!matchUploadLocation("cgi-bin", req.getConf(), loc)) {
			std::cerr << "Location not found for path: cgi-bin" << std::endl;
			return;
		}
		if (loc.uploadDirPath.empty()) {
			std::cerr << "Upload directory not set for path: cgi-bin" << std::endl;
			return;
		}
		if (loc.uploadDirPath[loc.uploadDirPath.size() - 1] != '/')
			filePath = loc.uploadDirPath + '/';
		filePath += fileName;
	}
    size_t dotPos = _path.find_last_of('.');
    if (dotPos != std::string::npos) {
        ext = _path.substr(dotPos + 1);
        if (ext == "php") {
            findPHP(_cgiBinPath, filePath);
        } 
        else if (ext == "py") {
            findPython(filePath);
        }
        else if (ext == "pl") {
            findPl(filePath);
        }
        else if (ext == "cgi") {
            findBash(filePath);
        }
        
        else {
            std::cerr << "Unsupported script type: " << ext << std::endl;
        }
    }
    _env.clear();
    
    std::vector<std::string> tempEnv;

    // For PHP-CGI
    std::string abs_path = makeAbsolutePath(_path);
    tempEnv.push_back("SCRIPT_FILENAME=" + std::string(abs_path));
    tempEnv.push_back("REDIRECT_STATUS=200");

    tempEnv.push_back("SERVER_SOFTWARE=WebServ/1.0");
    tempEnv.push_back("SERVER_NAME=WebServ/1.0");
    tempEnv.push_back("GATEWAY_INTERFACE=CGI/1.1");
    tempEnv.push_back("SERVER_PROTOCOL=WebServ/1.0");
    tempEnv.push_back("SERVER_PORT=" + tostring(_server->getConfParser().getPort(req.getConf())));
    tempEnv.push_back("REQUEST_METHOD=" + req.getMethod());
    tempEnv.push_back("PATH_INFO=" + getInfoPath());
    size_t slashPos = _path.find_last_of('/');
    std::string script;
    if (slashPos != std::string::npos) {
        script = _path.substr(slashPos + 1);
        tempEnv.push_back("SCRIPT_NAME=" + script);
    }
    tempEnv.push_back("QUERY_STRING=" + req.getQuery());
    tempEnv.push_back("CONTENT_TYPE=" + req.getContentType());
    tempEnv.push_back("CONTENT_LENGTH=" + tostring(req.getBody().length()));
    tempEnv.push_back("SCRIPT_NAME=" + req.getPath());
    tempEnv.push_back("SERVER_SOFTWARE=WebServ/1.0");

    for (std::vector<std::string>::iterator it = tempEnv.begin(); it != tempEnv.end(); ++it) {
        char* envStr = strdup(it->c_str());
        if (envStr) {
            _env.push_back(envStr);
        }
    }
    _env.push_back(NULL);
}

std::string CGIHandler::makeAbsolutePath(const std::string& path) {
    if (path.empty()) {
        return "";
    }

    if (path[0] == '/') {
        return path;
    }
    
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return path;
    }
    
    return std::string(cwd) + "/" + path;
}

void CGIHandler::setPathInfo(Request& req) {
    if (_path.empty()) {
        std::cout << "Error: _path is empty! Using requestPath instead.\n";
        _path = req.getPath();
    }
    
    size_t extPos = _path.find(".");
    size_t dotPos = 0;
    if (extPos == std::string::npos) {
        if ((extPos!= std::string::npos)) {
            dotPos = _path.length() - _path.find(".");
        } else {
            std::cout << "Warning: No valid extention found in request path!\n";
            _pathInfo = "";
            return ;
        }
    }
    
    size_t scriptPathEnd = extPos + dotPos;
    
    std::string scriptPath = _path.substr(0, scriptPathEnd);
    if (scriptPathEnd < _path.length()) {
        size_t queryPos = _path.find('?', scriptPathEnd);
        if (queryPos != std::string::npos) {
            _pathInfo = _path.substr(scriptPathEnd, queryPos - scriptPathEnd);
			std::cout << "Path info: " << _pathInfo << std::endl;
        } else {
            _pathInfo = _path.substr(scriptPathEnd);
        }
    } else {
        _pathInfo = "";
    }
}

void    CGIHandler::findBash(std::string& filePath) {
    _args = new char*[4];

    static const char* cgiLocations[] = {
        "/usr/bin/bash",
        "/run/current-system/sw/bin/bash", // NIXOS
        NULL
    };

    std::string cgiPath = "/usr/bin/env";
    for (int i = 0; cgiLocations[i] != NULL; ++i) {
        if (access(cgiLocations[i], X_OK) == 0) {
            cgiPath = cgiLocations[i];
            break;
        }
    }
	_args[0] = strdup(cgiPath.c_str());
    _args[1] = strdup(_path.c_str());
    _args[2] = NULL;
	if (!filePath.empty())
		_args[2] = strdup(filePath.c_str());
	_args[3] = NULL;
}

void    CGIHandler::findPython(std::string& filePath) {
    static const char* pythonLocations[] = {
        "/usr/bin/python3",
        "/run/current-system/sw/bin/python3",
        "/etc/profiles/per-user/schmitzi/bin/python3",
        "/usr/local/bin/python3",
        NULL
    };
    
    std::string _pythonPath = "/usr/bin/env";
    for (int i = 0; pythonLocations[i] != NULL; ++i) {
        if (access(pythonLocations[i], X_OK) == 0) {
            _pythonPath = pythonLocations[i];
            break;
        }
    }
    
    // Use env as fallback
    if (_pythonPath == "/usr/bin/env") {
        std::cout << "Python not found in whitelist, using env instead" << std::endl;
        _args = new char*[5];
        _args[0] = strdup(_pythonPath.c_str());
        _args[1] = strdup("python3");
        _args[2] = strdup(_path.c_str());
		_args[3] = NULL;
		if (!filePath.empty())
			_args[3] = strdup(filePath.c_str());
		_args[4] = NULL;
    } else {
        _args = new char*[4];
        _args[0] = strdup(_pythonPath.c_str());
        _args[1] = strdup(_path.c_str());
		_args[2] = NULL;
		if (!filePath.empty())
			_args[2] = strdup(filePath.c_str());
		_args[3] = NULL;
    }
}

void    CGIHandler::findPHP(std::string const &cgiBin, std::string& filePath) {
    std::string phpPath = cgiBin + "php-cgi";
    _args = new char*[4];
    _args[0] = strdup(phpPath.c_str());
    _args[1] = strdup("php-cgi");
	_args[2] = NULL;
	if (!filePath.empty())
		_args[2] = strdup(filePath.c_str());
	_args[3] = NULL;
}

void    CGIHandler::findPl(std::string& filePath) {
     // Whitelist of possible Perl locations
     static const char* perlLocations[] = {
        "/usr/bin/perl",
        "/run/current-system/sw/bin/perl",
        "/usr/local/bin/perl",
        NULL
    };
    
    std::string perlPath = "/usr/bin/env";
    for (int i = 0; perlLocations[i] != NULL; ++i) {
        if (access(perlLocations[i], X_OK) == 0) {
            perlPath = perlLocations[i];
            break;
        }
    }
    
    // Use env as fallback
    if (perlPath == "/usr/bin/env") {
        _args = new char*[5];
        _args[0] = strdup(perlPath.c_str());
        _args[1] = strdup("perl");
        _args[2] = strdup(_path.c_str());
		_args[3] = NULL;
		if (!filePath.empty())
			_args[3] = strdup(filePath.c_str());
        _args[4] = NULL;
    } else {
        _args = new char*[4];
        _args[0] = strdup(perlPath.c_str());
        _args[1] = strdup(_path.c_str());
		_args[2] = NULL;
		if (!filePath.empty())
			_args[2] = strdup(filePath.c_str());
		_args[3] = NULL;
    }
}


void CGIHandler::cleanupResources() {
    for (size_t i = 0; i < _env.size(); i++) {
        if (_env[i]) {
            free(_env[i]);
        }
    }
    _env.clear();
    
    if (_args) {
        for (int i = 0; _args[i]; i++) {
            free(_args[i]);
        }
        // delete[] _args;
        _args = NULL;
    }
    
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
}


Request    CGIHandler::createTempHeader(std::string output) {
    size_t headerEnd = output.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = output.find("\n\n");
    }

    Request temp;
    temp.setContentType("text/html");
    temp.setBody(output);
    std::string headers = "Content-Type: text/html\r\n";

    if (headerEnd != std::string::npos) {
        headers = output.substr(0, headerEnd);
        temp.setBody(output.substr(headerEnd + (output.find("\r\n\r\n") != std::string::npos ? 4 : 2)));

        size_t typePos = headers.find("Content-Type:");
        if (typePos != std::string::npos) {
            std::string tempType = temp.getContentType();
            size_t lineEnd = headers.find("\n", typePos);
            temp.setContentType(headers.substr(typePos + 13, lineEnd - typePos - 13));
            tempType.erase(0, tempType.find_first_not_of(" \t"));
            tempType.erase(tempType.find_last_not_of(" \t\r\n") + 1);
            temp.setContentType(tempType);
        }
    }
    return temp;
}

int CGIHandler::doChecks(Client client, Request& req) {
    if (access(_path.c_str(), F_OK) != 0) {
        std::cerr << getTimeStamp() << "Script does not exist: " << _path << std::endl;
        client.sendErrorResponse(404, req);
        return 1;
    }

    if (access(_path.c_str(), X_OK) != 0) {
        std::cerr << "Script is not executable: " << _path << std::endl;
        client.sendErrorResponse(403, req);
        return 1;
    }

    if (pipe(_input) < 0 || pipe(_output) < 0) {
        std::cerr << "Pipe creation failed" << std::endl;
        client.sendErrorResponse(500, req);
        return 1;
    }
    return 0;
}

std::string CGIHandler::getTimeStamp() {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    std::ostringstream oss;
    oss << "[" 
        << (tm_info->tm_year + 1900) << "-"
        << std::setw(2) << std::setfill('0') << (tm_info->tm_mon + 1) << "-"
        << std::setw(2) << std::setfill('0') << tm_info->tm_mday << " "
        << std::setw(2) << std::setfill('0') << tm_info->tm_hour << ":"
        << std::setw(2) << std::setfill('0') << tm_info->tm_min << ":"
        << std::setw(2) << std::setfill('0') << tm_info->tm_sec << "] ";
    
    return oss.str();
}