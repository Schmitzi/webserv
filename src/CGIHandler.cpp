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

void    CGIHandler::setClient(Client &client) {
    _client = &client;
}

void    CGIHandler::setServer(Server &server) {
    _server = &server;
}

std::string CGIHandler::getInfoPath() {
    return _pathInfo;
}

void    CGIHandler::setCGIBin(serverLevel *config) {
    _cgiBinPath = "";
    
    std::map<std::string, locationLevel>::iterator it = config->locations.begin();
    for (; it != config->locations.end(); ++it) {
        if (it->first.find("cgi-bin") != std::string::npos) {
            _cgiBinPath = it->second.cgiProcessorPath;
            break;
        }
    }
    
    if (_cgiBinPath.empty()) {
        if (NIX == true)
            _cgiBinPath = "/etc/profiles/per-user/schmitzi/bin/php-cgi";
        else
            _cgiBinPath = "/usr/bin/cgi-bin";
    }
}

int CGIHandler::executeCGI(Client &client, Request &req, std::string const &scriptPath) {
    cleanupResources();
    _path = scriptPath;
   
    if (doChecks(client, req) == 1)
        return 1;
    if (prepareEnv(req) == 1)
        return 1;

    // Convert string vectors to char* arrays for execve
    std::vector<char*> args_ptrs;
    for (size_t i = 0; i < _args.size(); i++) {
        args_ptrs.push_back(const_cast<char*>(_args[i].c_str()));
    }
    args_ptrs.push_back(NULL);
    
    std::vector<char*> env_ptrs;
    for (size_t i = 0; i < _env.size(); i++) {
        env_ptrs.push_back(const_cast<char*>(_env[i].c_str()));
    }
    env_ptrs.push_back(NULL);

    pid_t pid = fork();
    
    if (pid == 0) {  // Child process - unchanged
        close(_input[1]);
        close(_output[0]);
        
        if (dup2(_input[0], STDIN_FILENO) < 0 || 
            dup2(_output[1], STDOUT_FILENO) < 0) {
            std::cerr << getTimeStamp(_client->getFd()) << RED << "Error: dup2 failed" << RESET << std::endl;
            return 1;
        }
        close(_input[0]);
        close(_output[1]);
        
        execve(args_ptrs[0], args_ptrs.data(), env_ptrs.data());
        
        std::cerr << getTimeStamp(_client->getFd()) << RED << "Error: execve failed" << RESET << std::endl;
        cleanupResources();
        return 1;
    } 

    else if (pid > 0) {  // Parent process
        close(_input[0]);
        close(_output[1]);

        if (!req.getBody().empty()) {
			ssize_t w = write(_input[1], req.getBody().c_str(), req.getBody().length());
			if (!checkReturn(_client->getFd(), w, "write()", "Failed to write into pipe")) {
				close(_input[1]);
				_input[1] = -1;
				client.sendErrorResponse(500, req);//TODO: should this be sent?
				cleanupResources();
				return 1;
			}
        }
        close(_input[1]);
        _input[1] = -1;

        const int TIMEOUT_SECONDS = 30;
        time_t start_time = time(NULL);
        int status;
        
        while (true) {
            pid_t result = waitpid(pid, &status, WNOHANG);
            
            if (exit_status == 0) {
                std::cout << getTimeStamp(_client->getFd()) << BLUE << "CGI Script exit status: " << RESET << exit_status << std::endl;
                int result = processScriptOutput(client);
                cleanupResources();
                return result;
            } else {
                std::cerr << getTimeStamp(_client->getFd()) << RED << "CGI Script exit status: " << RESET << exit_status << std::endl;
                client.sendErrorResponse(500, req);
                cleanupResources();
                return 1;
            }
            
            if (time(NULL) - start_time > TIMEOUT_SECONDS) {
                std::cout << RED << getTimeStamp() << "CGI timeout, killing process " << pid << RESET << std::endl;
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0);
                client.sendErrorResponse(408, req);
                cleanupResources();
                return 1;
            }
            
            usleep(100000);
        }
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            std::cout << GREEN << getTimeStamp() << "CGI Script exit status: " << RESET << WEXITSTATUS(status) << "\n";
            int result = processScriptOutput(client);
            cleanupResources();
            return result;
        } else {
            std::cout << RED << getTimeStamp() << "CGI Script exit status: " << RESET << WEXITSTATUS(status) << "\n";
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
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    
    int selectResult = select(_output[0] + 1, &readfds, NULL, NULL, &timeout);
    
    if (selectResult > 0 && FD_ISSET(_output[0], &readfds)) {
        ssize_t bytesRead;
        while ((bytesRead = read(_output[0], buffer, sizeof(buffer) - 1)) >= 0) {
			if (bytesRead == 0)
				break;
            buffer[bytesRead] = '\0';
            output += buffer;
            totalBytesRead += bytesRead;
            std::cout << getTimeStamp(_client->getFd()) << BLUE << "Received bytes: " << RESET << bytesRead << BLUE << ", Total buffer: " << RESET << totalBytesRead << std::endl;
        }
		if (bytesRead < 0) {
			std::cerr << getTimeStamp(client.getFd()) << RED << "Error: read() failed" << RESET << std::endl;
			close(_output[0]);
    		_output[0] = -1;
			return 1;
		}
    }
    
    close(_output[0]);
    _output[0] = -1;
    std::cout << getTimeStamp(_client->getFd()) << BLUE << "Total bytes read: " << RESET << totalBytesRead << std::endl;

    if (output.empty()) {
        std::string defaultResponse = "HTTP/1.1 200 OK\r\n";
        defaultResponse += "Content-Type: text/plain\r\n";
        defaultResponse += "Content-Length: 22\r\n\r\n";
        defaultResponse += "No output from script\n";
        
        ssize_t x = send(client.getFd(), defaultResponse.c_str(), defaultResponse.length(), 0);
		if (!checkReturn(_client->getFd(), x, "send()", "Couldn't send default Response"))
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
    Request temp;
    
    std::map<std::string, std::string>::const_iterator typeIt = headerMap.find("Content-Type");
    if (typeIt != headerMap.end())
        temp.setContentType(typeIt->second);
    else
        temp.setContentType("text/html");
    
    temp.setBody(initialBody);
    _client->sendResponse(temp, "keep-alive", temp.getBody());
    return 0;
}

int CGIHandler::handleChunkedOutput(const std::map<std::string, std::string>& headerMap, const std::string& initialBody) {
    std::string response = "HTTP/1.1 200 OK\r\n";
    
    for (std::map<std::string, std::string>::const_iterator it = headerMap.begin(); it != headerMap.end(); ++it) {
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
        std::cerr << getTimeStamp(_client->getFd()) << RED << "Failed to send chunked response" << RESET << std::endl;
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
    std::string ext;
    std::string filePath;
    std::string fileName;
    std::string fileContent;
	locationLevel* loc = NULL;
    
	if (!req.getQuery().empty()) {
		doQueryStuff(req.getQuery(), fileName, fileContent);
        size_t dotPos = _path.find_last_of('.');
        if (dotPos != std::string::npos) {
            ext = "." + _path.substr(dotPos + 1); 
            
            if (!matchLocation(ext, req.getConf(), loc)) {
                std::cerr << getTimeStamp(_client->getFd()) << RED << "Location not found for extension: " << RESET << ext << std::endl;
                return 1;
            }
            
            if (loc->uploadDirPath.empty()) {
                std::cerr << getTimeStamp(_client->getFd()) << RED << "Upload directory not set for extension: " << RESET << ext << std::endl;
                return 1;
            }
            filePath = matchAndAppendPath(loc->uploadDirPath, fileName);
            
            makeArgs(loc->cgiProcessorPath, filePath);
        }
    } else {
        size_t dotPos = _path.find_last_of('.');
        if (dotPos != std::string::npos) {
            ext = "." + _path.substr(dotPos + 1);
            
            if (!matchLocation(ext, req.getConf(), loc)) {
                std::cerr << getTimeStamp(_client->getFd()) << RED << "Location not found for extension: " << RESET << ext << std::endl;
                return 1;
            }
            makeArgs(loc->cgiProcessorPath, filePath);
        }
    }
    
    _env.clear();
    
    const std::string abs_path = matchAndAppendPath(loc->rootLoc, _path);
    _env.push_back("SCRIPT_FILENAME=" + abs_path);
    _env.push_back("REDIRECT_STATUS=200");
    _env.push_back("SERVER_SOFTWARE=WebServ/1.0");
    _env.push_back("SERVER_NAME=" + req.getConf().servName[0]);//_env.push_back("SERVER_NAME=WebServ/1.0");//TODO: actual servername or this?
    _env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    _env.push_back("SERVER_PROTOCOL=HTTP/1.1");
    _env.push_back("SERVER_PORT=" + tostring(_server->getConfParser().getPort(req.getConf())));
    _env.push_back("REQUEST_METHOD=" + req.getMethod());
    _env.push_back("PATH_INFO=" + getInfoPath());
    
    size_t slashPos = _path.find_last_of('/');
    std::string script;
    if (slashPos != std::string::npos) {
        script = _path.substr(slashPos + 1);
        _env.push_back("SCRIPT_NAME=/" + script);
    }
    _env.push_back("QUERY_STRING=" + req.getQuery());
    _env.push_back("CONTENT_TYPE=" + req.getContentType());
    _env.push_back("CONTENT_LENGTH=" + tostring(req.getBody().length()));
    
    return 0;
}

void    CGIHandler::makeArgs(std::string const &cgiBin, std::string& filePath) {
    _args.clear();  
    _args.push_back(cgiBin);
    _args.push_back(_path);
    if (!filePath.empty()) {
        _args.push_back(filePath);
    }
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
}

int CGIHandler::doChecks(Client client, Request& req) {
    if (access(_path.c_str(), F_OK) != 0) {
        std::cerr << getTimeStamp(_client->getFd()) << RED << "Script does not exist: " << RESET << _path << std::endl;
        client.sendErrorResponse(404, req);
        return 1;
    }

    if (access(_path.c_str(), X_OK) != 0) {
        std::cerr << getTimeStamp(_client->getFd()) << RED << "Script is not executable: " << RESET << _path << std::endl;
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
