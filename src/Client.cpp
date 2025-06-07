#include "../include/Client.hpp"
#include "../include/Server.hpp"
#include "../include/Webserv.hpp"

Client::Client(Server& serv) {
	_addr = serv.getAddr();
	_fd = serv.getFd();
    std::cout << "FD: " << _fd << "\n";
    setWebserv(&serv.getWebServ());
    setServer(&serv);
	setConfigs(serv.getConfigs());
	_cgi = new CGIHandler();
	_cgi->setClient(*this);
	_cgi->setServer(serv);
}


Client::Client(const Client& client) {
    _addr = client._addr;        
    _fd = client._fd;            
    _addrLen = client._addrLen;
    _requestBuffer = client._requestBuffer; 
    _autoindex = client._autoindex;
    _webserv = client._webserv;
    _server = client._server;
    _configs = client._configs;
    _cgi = new CGIHandler();
    _cgi->setClient(*this);
    _cgi->setServer(*_server); 
}

Client& Client::operator=(const Client& other) {
    if (this != &other) {
        delete _cgi;
        
        _addr = other._addr;
        _fd = other._fd;
        _addrLen = other._addrLen;
        _requestBuffer = other._requestBuffer;
        _autoindex = other._autoindex;
        _webserv = other._webserv;
        _server = other._server;
        _configs = other._configs;
        
        _cgi = new CGIHandler();
        _cgi->setClient(*this);
        if (_server) {
            _cgi->setServer(*_server);
        }
    }
    return *this;
}

Client::~Client() {
	//delete _cgi; // TODO: this crashes the program
}

int     &Client::getFd() {
    return _fd;
}

void Client::setWebserv(Webserv &webserv) {
    _webserv = &webserv;
}

void Client::setServer(Server &server) {
    _server = &server;
}

Server &Client::getServer() {
    return *_server;
}

void    Client::setConfigs(const std::vector<serverLevel> &configs) {
    _configs = configs;
}

void    Client::setAutoIndex(locationLevel& loc) {
	if (loc.autoindex == true) {
		_autoindex = true;
	} else {
		_autoindex = false;
	}
}

int Client::acceptConnection(int serverFd) {
    _addrLen = sizeof(_addr);
    int newFd = accept(serverFd, (struct sockaddr *)&_addr, &_addrLen);
    if (newFd < 0) {
        _webserv->ft_error("Accept failed");
        return 1;
    }
    
    _fd = newFd;
    setConfigs(_server->getConfigs());
    _cgi->setServer(*_server);
    
    std::cout << BLUE << _webserv->getTimeStamp() << "Client accepted with fd: " << _fd << RESET << std::endl;
    
    return 0;
}

void Client::displayConnection() {
    unsigned char *ip = (unsigned char *)&_addr.sin_addr.s_addr;
    std::cout << BLUE << _webserv->getTimeStamp() << "New connection from " << RESET
                << (int)ip[0] << "."
                << (int)ip[1] << "."
                << (int)ip[2] << "."
                << (int)ip[3]
                << ":" << ntohs(_addr.sin_port) << "\n";
}

int Client::recieveData() {
    char buffer[1000000];
    memset(buffer, 0, sizeof(buffer));
    
    int bytesRead = recv(_fd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
    
    if (bytesRead > 0) {
        _requestBuffer.append(buffer, bytesRead);

        std::cout << BLUE << _webserv->getTimeStamp() 
                  << "Received " << bytesRead << " bytes from " << _fd 
                  << ", Total buffer: " << _requestBuffer.length() << " bytes" << RESET << "\n";

        Request req(_requestBuffer, getServer());
        
        bool isChunked = (_requestBuffer.find("Transfer-Encoding:") != std::string::npos &&
                          _requestBuffer.find("chunked") != std::string::npos);
        
        if (isChunked) {
            if (!isChunkedBodyComplete(_requestBuffer)) {
                std::cout << BLUE << _webserv->getTimeStamp() 
                          << "Chunked body incomplete, waiting for more data" << RESET << std::endl;
                return 0;
            }
        } else {
            size_t bodyStart = _requestBuffer.find("\r\n\r\n");
            if (bodyStart == std::string::npos) {
                bodyStart = _requestBuffer.find("\n\n");
                if (bodyStart == std::string::npos) {
                    return 0;
                }
                bodyStart += 2;
            } else {
                bodyStart += 4;
            }
            
            size_t actualBodyLength = _requestBuffer.length() - bodyStart;

            if (actualBodyLength < req.getContentLength()) {
                std::cout << BLUE << _webserv->getTimeStamp() 
                          << "Body incomplete: got " << actualBodyLength 
                          << " bytes, expected " << req.getContentLength() << RESET << std::endl;
                return 0;
            }
        }

        std::cout << GREEN << _webserv->getTimeStamp() 
                  << "Complete request received, processing..." << RESET << std::endl;
        
        int processResult = processRequest(req);
        
        if (processResult != -1) {
            _requestBuffer.clear();
        }
        return processResult;
    } 
    else if (bytesRead == 0) {
        std::cout << BLUE << _webserv->getTimeStamp() 
                  << "Client disconnected cleanly: " << _fd << RESET << "\n";
        return 1;
    }
    else {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        } 
        else if (errno == ECONNRESET) {
            std::cout << BLUE << _webserv->getTimeStamp() 
                      << "Connection reset by peer on fd " << _fd << RESET << "\n";
            return 1;
        }
        else if (errno == EPIPE) {
            std::cout << RED << _webserv->getTimeStamp() 
                      << "Broken pipe on fd " << _fd << RESET << "\n";
            return 1;
        }
        else {
            std::cout << RED << _webserv->getTimeStamp() 
                      << "recv() error on fd " << _fd << ": " << strerror(errno) << RESET << "\n";
            return 1;
        }
    }
}

bool Client::isChunkedBodyComplete(const std::string& buffer) {
    size_t bodyStart = buffer.find("\r\n\r\n");
    if (bodyStart == std::string::npos) {
        bodyStart = buffer.find("\n\n");
        if (bodyStart == std::string::npos) {
            return false;
        }
        bodyStart += 2;
    } else {
        bodyStart += 4;
    }
    
    if (bodyStart >= buffer.length()) {
        return false;
    }
    
    std::string body = buffer.substr(bodyStart);
    
    if (body.find("0\r\n\r\n") != std::string::npos || 
        body.find("0\n\n") != std::string::npos) {
        std::cout << GREEN << _webserv->getTimeStamp() 
                  << "Chunked body terminator found" << RESET << std::endl;
        return true;
    }
    
    return false;
}

int Client::processRequest(Request &req) {
	serverLevel &conf = req.getConf();
    if (req.getContentLength() > conf.requestLimit) {
        std::cerr << RED << _webserv->getTimeStamp() << "Content-Length too large" << RESET << std::endl;
        sendErrorResponse(413, req);
        _requestBuffer.clear();
        return 1;
    }

    if (req.getMethod() == "BAD") {
        std::cout << RED << _webserv->getTimeStamp() 
                << "Bad request format" << RESET << std::endl;
        sendErrorResponse(400, req);
        _requestBuffer.clear();
        return 1;
    }
    
    if (req.getMethod() != "GET" && req.getMethod() != "POST" && req.getMethod() != "DELETE") {
        std::cout << RED << _webserv->getTimeStamp() 
                << "Method not allowed: " << req.getMethod() << RESET << std::endl;
        sendErrorResponse(405, req);
        _requestBuffer.clear();
        return 1;
    }

    locationLevel loc = locationLevel();
    if (matchLocation(req.getPath(), conf, loc)) {
        if (loc.hasRedirect == true) {
            sendRedirect(loc.redirectionHTTP.first, loc.redirectionHTTP.second);
            return 0;
        }
    }
    
    if (req.getContentType().find("multipart/form-data") != std::string::npos) {
        int result = handleMultipartPost(req);
        
        if (result != -1) {
            return result;
        }
        return 0;
    }

    std::cout << BLUE << _webserv->getTimeStamp();
    std::cout << "Parsed Request: " << RESET << req.getMethod() << " " << req.getPath() << " " << req.getVersion() << "\n";

    if (req.getMethod() == "GET") {
        return handleGetRequest(req);
    } else if (req.getMethod() == "POST") {
        return handlePostRequest(req);
    } else if (req.getMethod() == "DELETE") {
        return handleDeleteRequest(req);
    } else {
        sendErrorResponse(405, req);
        return 1;
    }
}

int Client::handleGetRequest(Request& req) {
	std::string requestPath = req.getPath();
	locationLevel loc = locationLevel();
	if (!matchLocation(req.getReqPath(), req.getConf(), loc)) {
		std::cout << RED << _webserv->getTimeStamp() << "Location not found: " << RESET << req.getReqPath() << std::endl;
		sendErrorResponse(404, req);
		return 1;
	}
    if (requestPath.find("../") != std::string::npos) {
        sendErrorResponse(403, req);
        return 1;
    }
    setAutoIndex(loc);
    if (_autoindex == true && isFileBrowserRequest(requestPath)) {
        return handleFileBrowserRequest(req, requestPath);
    } else {
        return handleRegularRequest(req);
    }
}

bool Client::isFileBrowserRequest(const std::string& path) {
    return (path.length() >= 6 && path.substr(0, 6) == "/root/") || 
           (path == "/root");
}

int Client::handleFileBrowserRequest(Request& req, const std::string& requestPath) {
    std::string actualPath;
    if (requestPath == "/root" || requestPath == "/root/") {
        actualPath = "/";
    } else if (requestPath.find("/root/") == 0) {
        actualPath = requestPath.substr(5);
        if (actualPath.empty()) actualPath = "/";
    } else {
		locationLevel loc = locationLevel();
		matchLocation(requestPath, req.getConf(), loc);
        actualPath = requestPath.substr(5);
        std::string actualFullPath = _server->getWebRoot(req, loc) + actualPath;
        
        struct stat fileStat;
        if (stat(actualFullPath.c_str(), &fileStat) != 0) {
            std::cout << RED << _webserv->getTimeStamp() << "File not found: " << RESET << actualFullPath  << "\n";
            sendErrorResponse(404, req);
            return 1;
        }
        if (S_ISDIR(fileStat.st_mode)) {
            return createDirList(actualFullPath, actualPath, req);
        } else if (S_ISREG(fileStat.st_mode)) {
            if (buildBody(req, actualFullPath) == 1) {
                return 1;
            }
            
            req.setContentType(req.getMimeType(actualFullPath));
            sendResponse(req, "keep-alive", req.getBody());
            std::cout << GREEN << _webserv->getTimeStamp() << "Successfully served file from browser: " 
                    << RESET << actualFullPath << std::endl;
            return 0;
        } else {
            std::cout << _webserv->getTimeStamp() << "Not a regular file or directory: " << actualFullPath << std::endl;
            sendErrorResponse(403, req);
            return 1;
        }
    }
	return 0;
}

int Client::handleRegularRequest(Request& req) {
    locationLevel loc = locationLevel();

    if (!matchLocation(req.getPath(), req.getConf(), loc)) {
        std::cout << RED << _webserv->getTimeStamp() << "Location not found: " << RESET << req.getPath() << std::endl;
        sendErrorResponse(404, req);
        return 1;
    }

    std::string reqPath = req.getPath();
    if (reqPath == "/" || reqPath.empty()) {
        reqPath = loc.indexFile;
    }

    size_t end = reqPath.find_last_not_of(" \t\r\n");
    if (end != std::string::npos) {
        reqPath = reqPath.substr(0, end + 1);
    }
    std::string fullPath;
    if (reqPath.find("/home") == std::string::npos) {
        fullPath = _server->getWebRoot(req, loc) + reqPath;
    } else {
        fullPath = reqPath;
    }

	setAutoIndex(loc);

    if (fullPath.find("root") != std::string::npos && _autoindex == false) {
        std::cout << RED << "Access to directory browser is forbidden when autoindex is off\n" << RESET;
        sendErrorResponse(403, req);
        return 1;
    }

    if (handleRedirect(req) == 0) {
        return 1;
    }
	_cgi->setCGIBin(&req.getConf());
	_cgi->setClient(*this);
    if (_cgi->isCGIScript(reqPath)) {
		std::string fullCgiPath = _server->getWebRoot(req, loc) + reqPath;
        return _cgi->executeCGI(*this, req, fullCgiPath);
    }

    std::cout << BLUE << _webserv->getTimeStamp() << "Handling GET request for path: " << RESET << req.getPath() + "\n";

    struct stat fileStat;
    if (stat(fullPath.c_str(), &fileStat) != 0) {
        std::cout << RED << _webserv->getTimeStamp() << "File not found: " << RESET << fullPath << "\n";
        sendErrorResponse(404, req);
        return 1;
    }
    
    if (S_ISDIR(fileStat.st_mode)) {
        return viewDirectory(fullPath, req);
    } else if (!S_ISREG(fileStat.st_mode)) {
        std::cout << _webserv->getTimeStamp() << "Not a regular file: " << fullPath << std::endl;
        sendErrorResponse(403, req);
        return 1;
    }
    
    if (buildBody(req, fullPath) == 1) {
        return 1;
    }

    std::string contentType = req.getMimeType(fullPath);
    if (fullPath.find(".html") != std::string::npos || 
        reqPath == "/" || 
        reqPath == loc.indexFile) {
        contentType = "text/html";
    }

    req.setContentType(contentType);

    sendResponse(req, "close", req.getBody());
    std::cout << GREEN << _webserv->getTimeStamp() << "Successfully sent file: " << RESET << fullPath << std::endl;
    return 0;
}

int Client::buildBody(Request &req, std::string fullPath) {
    int fd = open(fullPath.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cout << _webserv->getTimeStamp() << "Failed to open file: " << fullPath << std::endl;
        sendErrorResponse(500, req);
        return 1;
    }
    
    struct stat fileStat;
    if (fstat(fd, &fileStat) < 0) {
        close(fd);
        sendErrorResponse(500, req);
        return 1;
    }
    
    std::vector<char> buffer(fileStat.st_size);

    ssize_t bytesRead = read(fd, buffer.data(), fileStat.st_size);
    close(fd);

    if (bytesRead < 0) {
        std::cout << "Error reading file: " << fullPath << std::endl;
        sendErrorResponse(500, req);
        return 1;
    }
    
    std::string fileContent(buffer.data(), bytesRead);
    req.setBody(fileContent);
    
    return 0;
}

int Client::viewDirectory(std::string fullPath, Request& req) {
    locationLevel loc = locationLevel();
	if (matchLocation(req.getPath(), req.getConf(), loc)) {
		setAutoIndex(loc);
		if (_autoindex == true) {
			return createDirList(fullPath, req.getPath(), req);
		} else {
			std::string indexPath = fullPath;
			if (indexPath[indexPath.size() - 1] != '/')
				indexPath += "/";
            indexPath += loc.indexFile;
			struct stat indexStat;
			if (stat(indexPath.c_str(), &indexStat) == 0 && S_ISREG(indexStat.st_mode)) {

				Request req;
				req.setPath(indexPath);
				if (buildBody(req, indexPath) == 1) {
					return 1;
				}
				
				req.setContentType("text/html");
				sendResponse(req, "keep-alive", req.getBody());
				std::cout << GREEN << _webserv->getTimeStamp() << "Successfully served index file: " 
						<< RESET << indexPath << std::endl;
				return 0;
			} else {
				std::cout << _webserv->getTimeStamp() << "Directory listing not allowed and no index.html: " << fullPath << std::endl;
				sendErrorResponse(403, req);
				return 1;
			}
        }
    } 
    else if (req.getConf().locations.find("/") != req.getConf().locations.end()) {
        if (req.getConf().locations.find("/")->second.autoindex) {
            return createDirList(fullPath, req.getPath(), req);
        } else {

            std::string indexPath = fullPath + loc.indexFile;
            struct stat indexStat;
            if (stat(indexPath.c_str(), &indexStat) == 0 && S_ISREG(indexStat.st_mode)) {
                Request req;
                req.setPath(indexPath);
                if (buildBody(req, indexPath) == 1) {
                    return 1;
                }
                
                req.setContentType("text/html");
                sendResponse(req, "keep-alive", req.getBody());
                std::cout << GREEN << _webserv->getTimeStamp() << "Successfully served index file: " 
                        << RESET << indexPath << std::endl;
                return 0;
            } else {
                std::cout << _webserv->getTimeStamp() << "Directory listing not allowed and no index.html: " << fullPath << std::endl;
                sendErrorResponse(403, req);
                return 1;
            }
        }
    }
    else {
        std::cout << _webserv->getTimeStamp() << "No matching location for: " << req.getPath() << std::endl;
        sendErrorResponse(403, req);
        return 1;
    }
    return 0;
}

int Client::createDirList(std::string fullPath, std::string requestPath, Request& req) {
    std::string dirListing = showDir(fullPath, requestPath);
    if (dirListing.empty()) {
        sendErrorResponse(403, req);
        return 1;
    }
    
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + tostring(dirListing.length()) + "\r\n";
    response += "Connection: keep-alive\r\n";
    response += "\r\n";
    response += dirListing;
    send(_fd, response.c_str(), response.length(), 0);
    std::cout << GREEN << _webserv->getTimeStamp() << "Successfully sent directory listing: " << RESET << fullPath << std::endl;
    return 0;
}

std::string Client::showDir(const std::string& dirPath, const std::string& requestUri) {
    DIR* dir = opendir(dirPath.c_str());
    if (!dir) {
        return "";
    }
    
    std::string html = "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>Index of " + requestUri + "</title>\n"
        "    <style>\n"
        "        body { font-family: Arial, sans-serif; margin: 20px; line-height: 1.6; }\n"
        "        h1 { color: #333; border-bottom: 1px solid #eee; padding-bottom: 10px; }\n"
        "        ul { list-style-type: none; padding: 0; }\n"
        "        li { padding: 8px; border-bottom: 1px solid #f2f2f2; }\n"
        "        li:hover { background-color: #f8f8f8; }\n"
        "        a { text-decoration: none; color: #0366d6; }\n"
        "        a:hover { text-decoration: underline; }\n"
        "        .header { background-color: #f4f4f4; padding: 10px; margin-bottom: 20px; border-radius: 4px; }\n"
        "        .server-info { font-size: 12px; color: #777; margin-top: 20px; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"header\">\n"
        "        <h1>Index of " + requestUri + "</h1>\n"
        "    </div>\n"
        "    <ul>\n";
    
    if (requestUri != "/") {
        std::string parentUri = requestUri;
        
        if (parentUri[parentUri.length() - 1] == '/') {
            parentUri = parentUri.substr(0, parentUri.length() - 1);
        }
        
        size_t lastSlash = parentUri.find_last_of('/');
        if (lastSlash != std::string::npos) {
            parentUri = parentUri.substr(0, lastSlash + 1);
        } else {
            parentUri = "/";
        }
        
        html += "        <li><a href=\"" + parentUri + "\">Parent Directory</a></li>\n";
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        
        if (name == "." || name == "..") {
            continue;
        }
        
        std::string entryPath = dirPath + "/" + name;
        struct stat entryStat;
        if (stat(entryPath.c_str(), &entryStat) == 0) {
            if (S_ISDIR(entryStat.st_mode)) {
                name += "/"; // Add trailing slash for directories
            }
        }
        
        html += "        <li><a href=\"" + requestUri;
        if (requestUri[requestUri.length() - 1] != '/') {
            html += "/";
        }
        html += name + "\">" + name + "</a></li>\n";
    }
    
    html += "    </ul>\n"
        "    <div class=\"server-info\">\n"
        "        <p>WebServ 1.0</p>\n"
        "    </div>\n"
        "</body>\n"
        "</html>";
        
    closedir(dir);
    return html;
}

std::string Client::extractFileName(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    
    if (pos == std::string::npos) {
        return path;
    }
    
    return path.substr(pos + 1);
}

std::string Client::getLocationPath(Request& req, const std::string& method) {	
	locationLevel loc = locationLevel();
	if (req.getPath().empty()) {
		std::cout << "Request path is empty for " << method << " request" << std::endl;
		sendErrorResponse(400, req);
		return "";
	}
	if (!matchUploadLocation(req.getPath(), req.getConf(), loc)) {
		std::cout << "Location not found for " << method << " request: " << req.getPath() << std::endl;
		sendErrorResponse(404, req);
		return "";
	}
	for (size_t i = 0; i < loc.methods.size(); i++) {
		if (loc.methods[i] == method) {
			break;
		}
		if (i == loc.methods.size() - 1) {
			std::cout << "Method not allowed for " << method << " request: " << req.getPath() << std::endl;
			sendErrorResponse(405, req);
			return "";
		}
	}
	if (loc.uploadDirPath.empty()) {
		std::cout << "Upload directory not set for " << method << " request: " << req.getPath() << std::endl;
		sendErrorResponse(403, req);
		return "";
	}
    std::string fullPath = loc.uploadDirPath;
    if (!fullPath.empty() && fullPath[fullPath.size() - 1] != '/')
        fullPath += "/";
    std::string fileName = extractFileName(req.getPath());
    fullPath += fileName;
    return fullPath;
}

int Client::handlePostRequest(Request& req) {
	locationLevel loc = locationLevel();
    if (!matchLocation(req.getPath(), req.getConf(), loc)) {
		std::cout << RED << _webserv->getTimeStamp() << "Location not found for POST request: " 
				  << RESET << req.getPath() << std::endl;
		sendErrorResponse(404, req);
		return 1;
	}
    std::string fullPath = getLocationPath(req, "POST");
    if (fullPath.empty())
        return 1;
    
    std::string cgiPath = _server->getWebRoot(req, loc) + req.getPath();
    if (_cgi->isCGIScript(cgiPath)) {
        return _cgi->executeCGI(*this, req, cgiPath);
    }
    
    if (req.getContentType().find("multipart/form-data") != std::string::npos) {
        return handleMultipartPost(req);
    }
    
    if (req.getPath().find("../") != std::string::npos) {
        sendErrorResponse(403, req);
        return 1;
    }
    
    std::string contentToWrite;
    
    if (isChunkedRequest(req)) {
        std::cout << BLUE << _webserv->getTimeStamp() << "Processing chunked request" << RESET << std::endl;
        contentToWrite = decodeChunkedBody(req.getBody());
        
        if (contentToWrite.empty()) {
            std::cout << RED << _webserv->getTimeStamp() << "Failed to decode chunked data" << RESET << std::endl;
            sendErrorResponse(400, req);
            return 1;
        }
    } else {
        contentToWrite = req.getBody();
        
        if (contentToWrite.empty() && !req.getQuery().empty()) {
			size_t pos = req.getQuery().find("=");
			if (pos != std::string::npos) {
				std::string key = req.getQuery().substr(0, pos);
				std::string value = req.getQuery().substr(pos + 1);
				if (key == "value" || key == "data")
					contentToWrite = value;
				else
					contentToWrite = value;
			}
			else
	            contentToWrite = req.getQuery();
        }
    }
    
    int fd = open(fullPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        std::cout << _webserv->getTimeStamp() << "Failed to open file for writing: " << fullPath << std::endl;
        sendErrorResponse(500, req);
        return 1;
    }
    
    std::cout << BLUE << _webserv->getTimeStamp() << "Writing to file: " << RESET << fullPath << "\n";

    
    ssize_t bytesWritten = write(fd, contentToWrite.c_str(), contentToWrite.length());
    close(fd);
    
    if (bytesWritten < 0) {
        std::cout << "Failed to write to file\n";
        sendErrorResponse(500, req);
        return 1;
    }
    
    std::cout << BLUE << _webserv->getTimeStamp() << "Sending success response..." << RESET << std::endl;
    
    std::string responseBody = "File uploaded successfully. Wrote " + tostring(bytesWritten) + " bytes.";
    
    ssize_t responseResult = sendResponse(req, "keep-alive", responseBody);
    
    if (responseResult < 0) {
        std::cout << RED << _webserv->getTimeStamp() << "Failed to send response" << RESET << std::endl;
        return 1;
    }
    
    std::cout << GREEN << _webserv->getTimeStamp() << "Successfully uploaded file: " << fullPath 
              << " (" << bytesWritten << " bytes written)" << RESET << std::endl;
    
    return 0;
}

int Client::handleDeleteRequest(Request& req) {
	std::string fullPath = getLocationPath(req, "DELETE");
	if (fullPath.empty())
		return 1;
    if (_cgi->isCGIScript(req.getPath())) {
        return _cgi->executeCGI(*this, req, fullPath);
    }

    size_t end = fullPath.find_last_not_of(" \t\r\n");
    if (end != std::string::npos) {
        fullPath = fullPath.substr(0, end + 1);
    }

    if (unlink(fullPath.c_str()) != 0) {
		if (errno == ENOENT) {
			std::cout << RED << _webserv->getTimeStamp() << "File not found for deletion: " << RESET << fullPath << "\n";
			sendErrorResponse(404, req);
			return 1;
		} else if (errno == EACCES || errno == EPERM) {
			std::cout << RED << _webserv->getTimeStamp() << "Permission denied for deletion: " << RESET << fullPath << "\n";
			sendErrorResponse(403, req);
			return 1;
		} else {
			std::cout << RED << _webserv->getTimeStamp() << "Error deleting file: " << RESET << fullPath
					  << " - " << strerror(errno) << "\n";
        	sendErrorResponse(500, req);
        	return 1;
		}
    }

    sendResponse(req, "keep-alive", "");
    return 0;
}

int Client::handleMultipartPost(Request& req) {
    std::string boundary = req.getBoundary();
    
    if (boundary.empty()) {
        sendErrorResponse(400, req);
        return 1;
    }
    
    Multipart parser(_requestBuffer, boundary);
    
    if (!parser.parse()) {
        sendErrorResponse(400, req);
        return 1;
    }
    
    std::string filename = parser.getFilename();
    if (filename.empty()) {
        sendErrorResponse(400, req);
        return 1;
    }

    std::string fileContent = parser.getFileContent();
    if (fileContent.empty() && !parser.isComplete()) {
        return -1;
    }
    
    if (!saveFile(req, filename, fileContent)) {
        sendErrorResponse(500, req);
        return 1;
    }
    std::cout << GREEN << _webserv->getTimeStamp() << "Recieved: " + parser.getFilename() << "\n" << RESET;
    std::string successMsg = "File uploaded successfully: " + filename;
    sendResponse(req, "close", successMsg);
    std::cout << GREEN << _webserv->getTimeStamp() << "File transfer ended\n" << RESET;    
    return 0;
}

bool Client::ensureUploadDirectory(Request& req) {
    struct stat st;
	std::string uploadDir = _server->getUploadDir(*this, req);
    if (stat(uploadDir.c_str(), &st) != 0) {
		std::cout << "Creating upload directory: " << uploadDir.c_str() << std::endl;
        if (mkdir(uploadDir.c_str(), 0755) != 0) {
            std::cout << "Error: Failed to create upload directory" << std::endl;
            return false;
        }
    }
    return true;
}

bool Client::saveFile(Request& req, const std::string& filename, const std::string& content) {
    std::string uploadDir = _server->getUploadDir(*this, req);
	if (uploadDir[uploadDir.size() - 1] != '/')
		uploadDir += "/";
	std::string fullPath = uploadDir + filename;
	if (fullPath.empty())
		return false;
	if (!ensureUploadDirectory(req)) {
		std::cout << "Error: Failed to ensure upload directory exists" << std::endl;
		return false;
	}
    
    int fd = open(fullPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        std::cout << "Error: Failed to open file for writing: " << fullPath << std::endl;
        return false;
    }
    
    ssize_t bytesWritten = write(fd, content.c_str(), content.length());
    close(fd);
    
    if (bytesWritten < 0) {
        std::cout << "Error: Failed to write to file" << std::endl;
        return false;
    }
    return true;
}

int    Client::handleRedirect(Request req) {
    std::string path = req.getPath().substr(1);
    std::map<std::string, locationLevel>::iterator it = req.getConf().locations.begin();
    for ( ; it != req.getConf().locations.end() ; it++) {
        if (it->first == path) {
            sendRedirect(it->second.redirectionHTTP.first, it->second.redirectionHTTP.second);
            return 0;
        }
    }
    return 1;
}

void Client::sendRedirect(int statusCode, const std::string& location) {
    std::string statusText = getStatusMessage(statusCode);
    
    // Create HTML body first
    std::string body = "<!DOCTYPE html><html><head><title>" + statusText + "</title></head>";
    body += "<body><h1>" + statusText + "</h1>";
    body += "<p>The document has moved <a href=\"" + location + "\">here</a>.</p></body></html>";
    
    // Create complete response with all headers
    std::string response = "HTTP/1.1 " + tostring(statusCode) + " " + statusText + "\r\n";
    response += "Location: " + location + "\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + tostring(body.length()) + "\r\n";
    response += "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n";
    response += "Pragma: no-cache\r\n";
    response += "Connection: close\r\n";
    response += "\r\n"; // Empty line separating headers from body
    response += body;   // Add the body after headers
    
    std::cout << BLUE << _webserv->getTimeStamp() << "Sent redirect response: " << RESET
              << statusCode << " " << statusText << " to " << location << "\n";
    
    send(_fd, response.c_str(), response.length(), 0);
}

ssize_t Client::sendResponse(Request req, std::string connect, std::string body) {
 
    if (_fd <= 0) {
        std::cerr << "ERROR: Invalid file descriptor in sendResponse: " << _fd << std::endl;
        return -1;
    }
    
    std::string response = "HTTP/1.1 200 OK\r\n";
    
    std::map<std::string, std::string> headers = req.getHeaders();
    bool isChunked = false;
    std::map<std::string, std::string>::iterator it = headers.find("Transfer-Encoding");
    if (it != headers.end() && it->second.find("chunked") != std::string::npos) {
        isChunked = true;
    }
    
    response += "Content-Type: " + req.getContentType() + "\r\n";
    
    std::string content = body;
    
    if (req.getMethod() == "POST" && content.empty()) {
        content = "Upload successful";
    }
    
    if (!isChunked) {
        response += "Content-Length: " + tostring(content.length()) + "\r\n";
    } else {
        response += "Transfer-Encoding: chunked\r\n";
    }
    
    response += "Server: WebServ/1.0\r\n";
    response += "Connection: " + connect + "\r\n";
    response += "Access-Control-Allow-Origin: *\r\n";
    response += "\r\n";

    std::cout << BLUE << _webserv->getTimeStamp() << "Attempting to send " << response.length() 
              << " bytes to fd " << _fd << RESET << std::endl;
    
    // Validate fd again before sending
    if (_fd <= 0) {
        std::cerr << "ERROR: File descriptor became invalid before send: " << _fd << std::endl;
        return -1;
    }
    
    ssize_t headerBytes = send(_fd, response.c_str(), response.length(), 0);
    if (headerBytes < 0) {
        std::cerr << RED << _webserv->getTimeStamp() << "Failed to send headers to fd " << _fd 
                  << ": " << strerror(errno) << RESET << std::endl;
        return -1;
    }
    
    std::cout << GREEN << _webserv->getTimeStamp() << "Sent headers " << RESET <<"(" << headerBytes << " bytes)\n";
    
    if (!content.empty()) {
        if (isChunked) {
            const size_t chunkSize = 4096;
            size_t remaining = content.length();
            size_t offset = 0;
            
            while (remaining > 0) {
                size_t currentChunkSize;
                if (remaining < chunkSize) 
                    currentChunkSize = remaining;
                else 
                    currentChunkSize = chunkSize;
                
                std::stringstream hexStream;
                hexStream << std::hex << currentChunkSize;
                std::string chunkHeader = hexStream.str() + "\r\n";
                send(_fd, chunkHeader.c_str(), chunkHeader.length(), 0);
                send(_fd, content.c_str() + offset, currentChunkSize, 0);
                send(_fd, "\r\n", 2, 0);
                offset += currentChunkSize;
                remaining -= currentChunkSize;
            }
            
            send(_fd, "0\r\n\r\n", 5, 0);
            
            std::cout << GREEN << _webserv->getTimeStamp() << "Sent chunked body " << RESET << "(" << content.length() << " bytes)\n";
            return content.length();
        } else {
            ssize_t bodyBytes = send(_fd, content.c_str(), content.length(), 0);
            if (bodyBytes < 0) {
                std::cerr << RED << _webserv->getTimeStamp() << "Failed to send body" << RESET <<"\n";
                return -1;
            }
            std::cout << GREEN << _webserv->getTimeStamp() << "Sent body " << RESET << "(" << bodyBytes << " bytes)\n";
            return bodyBytes;
        }
    } else {
        std::cout << GREEN << _webserv->getTimeStamp() << "Response sent (headers only)" << RESET << "\n";
        return 0;
    }
}

bool Client::send_all(int sockfd, const std::string& data) {
	size_t total_sent = 0;
	size_t to_send = data.size();
	const char* buffer = data.c_str();

	while (total_sent < to_send) {
		ssize_t sent = send(sockfd, buffer + total_sent, to_send - total_sent, 0);
		if (sent <= 0)
			return false;
		total_sent += sent;
	}
	return true;
}

void Client::sendErrorResponse(int statusCode, Request& req) {
    std::string body;
    std::string statusText = getStatusMessage(statusCode);
    
    resolveErrorResponse(statusCode, statusText, body, req);    
    
    // Construct the HTTP response
    std::string response = "HTTP/1.1 " + tostring(statusCode) + " " + statusText + "\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + tostring(body.size()) + "\r\n";
    response += "Server: WebServ/1.0\r\n";
    response += "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n";
    response += "Pragma: no-cache\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += body;
    
    if (!send_all(_fd, response)) {
        std::cerr << RED << _webserv->getTimeStamp() << "Failed to send error response" << std::endl;
    }
    std::cerr << RED << _webserv->getTimeStamp() << "Error sent: " << statusCode << RESET << std::endl;
}

bool Client::isChunkedRequest(const Request& req) {
    std::map<std::string, std::string> headers = const_cast<Request&>(req).getHeaders();
    std::map<std::string, std::string>::iterator it = headers.find("Transfer-Encoding");
    if (it != headers.end() && it->second.find("chunked") != std::string::npos) {
        return true;
    }   
    return false;
}

std::string Client::decodeChunkedBody(const std::string& chunkedData) {
    std::string decodedBody;
    size_t pos = 0;
    
    std::cout << BLUE << _webserv->getTimeStamp() << "Decoding chunked data, total size: " 
              << chunkedData.length() << " bytes" << RESET << std::endl;
    
    while (pos < chunkedData.length()) {
        size_t crlfPos = chunkedData.find("\r\n", pos);
        if (crlfPos == std::string::npos) {
            crlfPos = chunkedData.find("\n", pos);
            if (crlfPos == std::string::npos) {
                std::cerr << RED << _webserv->getTimeStamp() << "Malformed chunked data: no CRLF after chunk size" << RESET << std::endl;
                break;
            }
        }
        
        std::string chunkSizeStr = chunkedData.substr(pos, crlfPos - pos);
        
        size_t semicolon = chunkSizeStr.find(';');
        if (semicolon != std::string::npos) {
            chunkSizeStr = chunkSizeStr.substr(0, semicolon);
        }
        
        chunkSizeStr.erase(0, chunkSizeStr.find_first_not_of(" \t"));
        chunkSizeStr.erase(chunkSizeStr.find_last_not_of(" \t") + 1);
        
        size_t chunkSize = 0;
        std::istringstream hexStream(chunkSizeStr);
        hexStream >> std::hex >> chunkSize;
        
        std::cout << BLUE << _webserv->getTimeStamp() << "Chunk size: 0x" << chunkSizeStr 
                  << " (" << chunkSize << " bytes)" << RESET << std::endl;
        
        if (chunkSize == 0) {
            std::cout << GREEN << _webserv->getTimeStamp() << "End of chunked data reached" << RESET << std::endl;
            break;
        }
        
        if (crlfPos + (chunkedData[crlfPos] == '\r')) {
            pos = 2;
        } else {
            pos = 1;
        }
        
        // Check if we have enough data for this chunk
        if (pos + chunkSize > chunkedData.length()) {
            std::cerr << BLUE << "Incomplete chunk data: expected " << chunkSize 
                      << " bytes, but only " << (chunkedData.length() - pos) << " available" << RESET << std::endl;
            break;
        }
        
        std::string chunkData = chunkedData.substr(pos, chunkSize);
        decodedBody += chunkData;
        
        std::cout << BLUE << _webserv->getTimeStamp() << "Decoded chunk: \"" 
                  << chunkData << "\"" << RESET << std::endl;
        
        pos += chunkSize;
        
        if (pos < chunkedData.length() && chunkedData[pos] == '\r') {
            pos++;
        }
        if (pos < chunkedData.length() && chunkedData[pos] == '\n') {
            pos++;
        }
    }
    
    std::cout << GREEN << _webserv->getTimeStamp() << "Total decoded body: \"" 
              << decodedBody << "\" (" << decodedBody.length() << " bytes)" << RESET << std::endl;
    
    return decodedBody;
}

