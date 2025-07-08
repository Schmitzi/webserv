#include "../include/Client.hpp"
#include "../include/Server.hpp"
#include "../include/Webserv.hpp"

Client::Client(Server& serv, uint32_t &eventMask) {
	_addr = serv.getAddr();
	_fd = serv.getFd();
    _webserv = &serv.getWebServ();
    _server = &serv;
	_configs = serv.getConfigs();
	_connect = "";
	_sendOffset = 0;
	_exitCode = 0;
    (void)eventMask;
}

Client::Client(const Client& client) {
    *this = client;
}

Client& Client::operator=(const Client& other) {
    if (this != &other) {
        _addr = other._addr;
        _fd = other._fd;
        _addrLen = other._addrLen;
        _requestBuffer = other._requestBuffer;
        _webserv = other._webserv;
        _server = other._server;
        _configs = other._configs;
		_connect = other._connect;
		_sendOffset = other._sendOffset;
		_exitCode = other._exitCode;
	}
    return *this;
}

Client::~Client() {}

int	&Client::getFd() {
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

std::vector<serverLevel> Client::getConfigs() {
	return _configs;
}

size_t& Client::getOffset() {
	return _sendOffset;
}

void Client::setConnect(std::string connect) {
	_connect = connect;
}

std::string Client::getConnect() {
	return _connect;
}

int Client::getExitCode() {
	return _exitCode;
}

void Client::setExitCode(int i) {
	_exitCode = i;
}

int Client::acceptConnection(int serverFd) {
    _addrLen = sizeof(_addr);
    _fd  = accept(serverFd, (struct sockaddr *)&_addr, &_addrLen);
    if (_fd < 0) {
		std::cerr << getTimeStamp() << RED << "Error: accept() failed" << RESET << std::endl;
        return 1;
    }
    
    std::cout << getTimeStamp(_fd) << "Client accepted" << std::endl;
    
    return 0;
}

void Client::displayConnection() {
    unsigned char *ip = (unsigned char *)&_addr.sin_addr.s_addr;
    std::cout << getTimeStamp(_fd) << BLUE << "New connection from "
        << (int)ip[0] << "." << (int)ip[1] << "." << (int)ip[2] << "."
            << (int)ip[3] << ":" << ntohs(_addr.sin_port) << RESET << std::endl;
}

int Client::recieveData() {
    static bool printNewLine = false;
    char buffer[1000000];
    memset(buffer, 0, sizeof(buffer));
    
    ssize_t bytesRead = recv(_fd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
    if (bytesRead < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        std::cerr << getTimeStamp(_fd) << RED << "Error: recv() failed" << RESET << std::endl;
        return 1;
    }
    
    if (bytesRead == 0) {
        std::cout << getTimeStamp(_fd) << BLUE << "Client closed connection" << RESET << std::endl;
        return 1;
    }
    
    _requestBuffer.append(buffer, bytesRead);

    bool isChunked = (_requestBuffer.find("Transfer-Encoding:") != std::string::npos &&
                        _requestBuffer.find("chunked") != std::string::npos);
    
    if (isChunked) {
        if (!isChunkedBodyComplete(_requestBuffer)) {
            std::cout << getTimeStamp(_fd) << BLUE 
                        << "Chunked body incomplete, waiting for more data" << RESET << std::endl;
            return 0;
        }
    } else {
        if (checkLength(printNewLine) == 0)
            return 0;
    }
    if (printNewLine == true)
        std::cout << std::endl;
    std::cout << getTimeStamp(_fd) << GREEN  << "Complete request received, processing..." << RESET << std::endl;
    printNewLine = false;
    
    if (_requestBuffer.empty() || _requestBuffer.find("HTTP/") == std::string::npos) {
        std::cout << getTimeStamp(_fd) << YELLOW << "Empty or invalid request received, closing connection" << RESET << std::endl;
        _requestBuffer.clear();
        return 1;
    }
    
    Request req(_requestBuffer, *this, _fd);
    int processResult = processRequest(req);
    
    if (processResult != 1)
        _requestBuffer.clear();
    _exitCode = processResult;
    return processResult;
}

int Client::checkLength(bool &printNewLine) {
    size_t contentLengthPos = _requestBuffer.find("Content-Length:");
    if (contentLengthPos != std::string::npos) {
        size_t valueStart = contentLengthPos + 15;
        while (valueStart < _requestBuffer.length() && 
                (_requestBuffer[valueStart] == ' ' || _requestBuffer[valueStart] == '\t')) {
            valueStart++;
        }
        
        size_t valueEnd = _requestBuffer.find("\r\n", valueStart);
        if (valueEnd == std::string::npos) {
            valueEnd = _requestBuffer.find("\n", valueStart);
        }
        
        if (valueEnd != std::string::npos) {
            std::string lengthStr = _requestBuffer.substr(valueStart, valueEnd - valueStart);
            size_t expectedLength = strtoul(lengthStr.c_str(), NULL, 10);
            
            size_t bodyStart = _requestBuffer.find("\r\n\r\n");
            if (bodyStart != std::string::npos) {
                bodyStart += 4;
            } else {
                bodyStart = _requestBuffer.find("\n\n");
                if (bodyStart != std::string::npos) {
                    bodyStart += 2;
                }
            }
            
            if (bodyStart != std::string::npos) {
                size_t actualBodyLength = _requestBuffer.length() - bodyStart;
                if (actualBodyLength < expectedLength) {
					if (printNewLine == false)
						std::cout << getTimeStamp(_fd) << BLUE << "Receiving bytes..." << RESET << std::endl;
					std::cout << "[~]";
					printNewLine = true;
                    return 0;
                }
            }
        }
    }
    return 1;
}

bool Client::isChunkedBodyComplete(const std::string& buffer) {
    size_t bodyStart = buffer.find("\r\n\r\n");
    if (bodyStart == std::string::npos) {
        bodyStart = buffer.find("\n\n");
        if (bodyStart == std::string::npos)
            return false;
        bodyStart += 2;
    } else
        bodyStart += 4;
    
    if (bodyStart >= buffer.length())
        return false;
    
    std::string body = buffer.substr(bodyStart);
    
    if (body.find("0\r\n\r\n") != std::string::npos || body.find("0\n\n") != std::string::npos)
        return true;
    
    return false;
}

int Client::processRequest(Request& req) {
	serverLevel &conf = req.getConf();
    if (req.getContentLength() > conf.requestLimit) {
        std::cerr << getTimeStamp(_fd) << RED  << "Content-Length too large" << RESET << std::endl;
        sendErrorResponse(413, req);
        _requestBuffer.clear();
        return 1;
    }

    if (req.getCheck() == "BAD") {
        std::cerr << getTimeStamp(_fd) << RED  << "Bad request format " << RESET << std::endl;
        sendErrorResponse(400, req);
        _requestBuffer.clear();
        return 1;
    }
    
    if (req.getCheck() == "NOTALLOWED") {
        std::cerr << getTimeStamp(_fd) << RED  << "Method Not Allowed: " << RESET << req.getMethod() << std::endl;
        sendErrorResponse(405, req);
        _requestBuffer.clear();
        return 1;
    }

    locationLevel* loc = NULL;
    if (matchLocation(req.getPath(), conf, loc)) {
        if (loc->hasRedirect == true) {
            sendRedirect(loc->redirectionHTTP.first, loc->redirectionHTTP.second);
            return 0;
        }
    }
    if (req.getContentType().find("multipart/form-data") != std::string::npos) {
        int result = handleMultipartPost(req);
        if (result != -1)
            return result;
        return 0;
    }
    std::cout << getTimeStamp(_fd) << BLUE << "Parsed Request: " << RESET << 
        req.getMethod() << " " << req.getPath() << " " << req.getVersion() << std::endl;

    if (req.getMethod() == "GET")
        return handleGetRequest(req);
    else if (req.getMethod() == "POST")
        return handlePostRequest(req);
    else if (req.getMethod() == "DELETE")
        return handleDeleteRequest(req);
    else {
        sendErrorResponse(405, req);
        return 1;
    }
}

int Client::handleGetRequest(Request& req) {
	std::string requestPath = req.getPath();
	locationLevel* loc = NULL;
	if (!matchLocation(req.getPath(), req.getConf(), loc)) {
		std::cerr << getTimeStamp(_fd) << RED  << "Location not found: " << RESET << req.getPath() << std::endl;
		sendErrorResponse(404, req);
		return 1;
	}
    if (requestPath.find("../") != std::string::npos) {
        sendErrorResponse(403, req);
        return 1;
    }
    if (loc->autoindex == true && isFileBrowserRequest(requestPath))
        return handleFileBrowserRequest(req);
    else
        return handleRegularRequest(req);
}

bool Client::isFileBrowserRequest(const std::string& path) {
    return (path.length() >= 6 && path.substr(0, 6) == "/root/") || (path == "/root");
}

int Client::handleFileBrowserRequest(Request& req) {
    std::string requestPath = req.getPath();
	std::string actualPath;
    if (requestPath == "/root" || requestPath == "/root/")
        actualPath = "/";
    else if (requestPath.find("/root/") == 0) {
        actualPath = requestPath.substr(5);
        if (actualPath.empty()) actualPath = "/";
    } else {
		locationLevel* loc = NULL;
		matchLocation(requestPath, req.getConf(), loc);
        actualPath = requestPath.substr(5);
        std::string actualFullPath = matchAndAppendPath(_server->getWebRoot(req, *loc), actualPath);
        
        struct stat fileStat;
        if (stat(actualFullPath.c_str(), &fileStat) != 0) {
            std::cerr << getTimeStamp(_fd) << RED  << "File not found: " << RESET << actualFullPath  << std::endl;
            sendErrorResponse(404, req);
            return 1;
        }
        if (S_ISDIR(fileStat.st_mode)) {
			req.setPath(actualPath);
            return createDirList(actualFullPath, req);
        } else if (S_ISREG(fileStat.st_mode)) {
            if (buildBody(req, actualFullPath) == 1)
                return 1;
            req.setContentType(req.getMimeType(actualFullPath));
            sendResponse(req, "keep-alive", req.getBody());
            std::cout << getTimeStamp(_fd) << GREEN  << "Successfully served file from browser: " 
                    << RESET << actualFullPath << std::endl;
            return 0;
        } else {
            std::cerr << getTimeStamp(_fd) << RED << "Not a regular file or directory: " << RESET << actualFullPath << std::endl;
            sendErrorResponse(403, req);
            return 1;
        }
    }
	return 0;
}

bool Client::isCGIScript(const std::string& path) {
    size_t dotPos = path.find_last_of('.');
    
    if (dotPos != std::string::npos) {
        std::string ext = path.substr(dotPos + 1);
        
        static const char* whiteList[] = {"py", "php", "cgi", "pl", NULL};
        
        for (int i = 0; whiteList[i] != NULL; ++i) {
            if (ext.find(whiteList[i]) != std::string::npos)
                return true;
        }
    }
    return false;
}

int Client::handleRegularRequest(Request& req) {
    locationLevel* loc = NULL;
    if (!matchLocation(req.getPath(), req.getConf(), loc)) {
        std::cerr << getTimeStamp(_fd) << RED  << "Location not found: " << RESET << req.getPath() << std::endl;
        sendErrorResponse(404, req);
        return 1;
    }
    std::string reqPath = req.getPath();
    if (reqPath == "/" || reqPath.empty())
        reqPath = loc->indexFile;

    size_t end = reqPath.find_last_not_of(" \t\r\n");
    if (end != std::string::npos)
        reqPath = reqPath.substr(0, end + 1);
    std::string fullPath;
    if (reqPath.find("/home") == std::string::npos)
        fullPath = matchAndAppendPath(_server->getWebRoot(req, *loc), reqPath);
    else
        fullPath = reqPath;

    if (fullPath.find("root") != std::string::npos && loc->autoindex == false) {
        std::cerr << getTimeStamp(_fd) << RED << "Access to directory browser is forbidden when autoindex is off" << RESET << std::endl;
        sendErrorResponse(403, req);
        return 1;
    }

    if (handleRedirect(req) == 0)
        return 1;
        
    if (isCGIScript(reqPath)) {      
        CGIHandler* cgi = new CGIHandler(*this);
        cgi->setCGIBin(&req.getConf());
        std::string fullCgiPath = matchAndAppendPath(_server->getWebRoot(req, *loc), reqPath);
        cgi->setPath(fullCgiPath);

        int result = cgi->executeCGI(req);
        
        if (result != 0) {
            delete cgi;
        }
        
        return result;
    }

    std::cout << getTimeStamp(_fd) << BLUE << "Handling GET request for path: " << RESET << req.getPath() << std::endl;

    struct stat fileStat;
    if (stat(fullPath.c_str(), &fileStat) != 0) {
        std::cerr << getTimeStamp(_fd) << RED  << "File not found: " << RESET << fullPath << std::endl;
        sendErrorResponse(404, req);
        return 1;
    }
    
    if (S_ISDIR(fileStat.st_mode))
        return viewDirectory(fullPath, req);
    else if (!S_ISREG(fileStat.st_mode)) {
        std::cerr << getTimeStamp(_fd) << RED << "Not a regular file: " << RESET << fullPath << std::endl;
        sendErrorResponse(403, req);
        return 1;
    }
    
    if (buildBody(req, fullPath) == 1)
        return 1;

    std::string contentType = req.getMimeType(fullPath);
    if (fullPath.find(".html") != std::string::npos || reqPath == "/" || reqPath == loc->indexFile)
        contentType = "text/html";

    req.setContentType(contentType);

    sendResponse(req, "close", req.getBody());
    std::cout << getTimeStamp(_fd) << GREEN  << "Sent file: " << RESET << fullPath << std::endl;
    return 0;
}

int Client::buildBody(Request &req, std::string fullPath) {
    int fd = open(fullPath.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cerr << getTimeStamp(_fd) << RED << "Failed to open file: " << RESET << fullPath << std::endl;
        sendErrorResponse(500, req);
        return 1;
    }
    
    struct stat fileStat;
    if (fstat(fd, &fileStat) < 0) {
		sendErrorResponse(500, req);
        close(fd);
        return 1;
    }
    
    std::vector<char> buffer(fileStat.st_size);

    ssize_t bytesRead = read(fd, buffer.data(), fileStat.st_size);
	if (bytesRead == 0) {
		close(fd);
		std::cout << getTimeStamp(_fd) << BLUE << "Nothing to be read in file: " << RESET << fullPath << std::endl;
		return 0;
	}
    else if (bytesRead < 0) {
        std::cerr << getTimeStamp(_fd) << RED << "Error: read() failed on file: " << RESET << fullPath << std::endl;
        sendErrorResponse(500, req);
		close(fd);
        return 1;
    }
	std::string fileContent(buffer.data(), bytesRead);
	req.setBody(fileContent);
	close(fd);
    return 0;
}

int Client::viewDirectory(std::string fullPath, Request& req) {
    locationLevel* loc = NULL;
	if (matchLocation(req.getPath(), req.getConf(), loc)) {
		if (loc->autoindex == true) {
			return createDirList(fullPath, req);
		} else {
			std::string indexPath = matchAndAppendPath(fullPath, loc->indexFile);
			struct stat indexStat;
			if (stat(indexPath.c_str(), &indexStat) == 0 && S_ISREG(indexStat.st_mode)) {
				Request req;
				req.setPath(indexPath);
				if (buildBody(req, indexPath) == 1)
					return 1;
				req.setContentType("text/html");
				_connect = "keep-alive";
				sendResponse(req, "keep-alive", req.getBody());
				std::cout << getTimeStamp(_fd) << GREEN  << "Successfully served index file: " << RESET << indexPath << std::endl;
				return 0;
			} else {
				std::cerr << getTimeStamp(_fd) << RED  << "Autoindex off and no index.html: " << RESET << fullPath << std::endl;
				sendErrorResponse(403, req);
				return 1;
			}
        }
    } 
    else if (req.getConf().locations.find("/") != req.getConf().locations.end()) {
        if (req.getConf().locations.find("/")->second.autoindex) {
            return createDirList(fullPath, req);
        } else {
            std::string indexPath = matchAndAppendPath(fullPath, loc->indexFile);
            struct stat indexStat;
            if (stat(indexPath.c_str(), &indexStat) == 0 && S_ISREG(indexStat.st_mode)) {
                Request req;
                req.setPath(indexPath);
                if (buildBody(req, indexPath) == 1)
                    return 1;
                req.setContentType("text/html");
				_connect = "keep-alive";
                sendResponse(req, "keep-alive", req.getBody());
                std::cout << getTimeStamp(_fd) << GREEN  << "Successfully served index file: " << RESET << indexPath << std::endl;
                return 0;
            } else {
                std::cerr << getTimeStamp(_fd) << RED  << "Autoindex off and no index.html: " << RESET << fullPath << std::endl;
                sendErrorResponse(403, req);
                return 1;
            }
        }
    }
    else {
        std::cerr << getTimeStamp(_fd) << RED << "No matching location for: " << RESET << req.getPath() << std::endl;
        sendErrorResponse(403, req);
        return 1;
    }
    return 0;
}

int Client::createDirList(std::string fullPath, Request& req) {
    std::string dirListing = showDir(fullPath, req.getPath());
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
	_connect = "keep-alive";
	_webserv->addSendBuf(_fd, response);
	_webserv->setEpollEvents(_fd, EPOLLOUT);
    std::cout << getTimeStamp(_fd) << GREEN  << "Sent directory listing: " << RESET << fullPath << std::endl;
    return 0;
}

std::string Client::showDir(const std::string& dirPath, const std::string& requestUri) {
    DIR* dir = opendir(dirPath.c_str());
    if (!dir)
        return "";
    
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
        std::string parentUri = encode(requestUri);
        parentUri = "/" + matchAndAppendPath(parentUri, "/../") + "/";
        html += "        <li><a href=\"" + parentUri + "\">Parent Directory</a></li>\n";
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        
        if (name == "." || name == "..")
            continue;
        
        std::string entryPath = dirPath + "/" + name;
        struct stat entryStat;
        if (stat(entryPath.c_str(), &entryStat) == 0) {
            if (S_ISDIR(entryStat.st_mode))
                name += "/";
        }
        
        html += "        <li><a href=\"" + encode(requestUri);
        if (requestUri[requestUri.length() - 1] != '/')
            html += "/";
        html += encode(name) + "\">" + name + "</a></li>\n";
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

std::string Client::getLocationPath(Request& req, const std::string& method) {	
	locationLevel* loc = NULL;
	if (req.getPath().empty()) {
		std::cerr << getTimeStamp(_fd) << RED << "Request path is empty for " << method << " request" << RESET << std::endl;
		sendErrorResponse(400, req);
		return "";
	}
	if (!matchUploadLocation(req.getPath(), req.getConf(), loc)) {
		std::cerr << getTimeStamp(_fd) << RED << "Location not found for " << method << " request: " << RESET << req.getPath() << std::endl;
		sendErrorResponse(404, req);
		return "";
	}
	for (size_t i = 0; i < loc->methods.size(); i++) {
		if (loc->methods[i] == method)
			break;
		if (i == loc->methods.size() - 1) {
			std::cerr << getTimeStamp(_fd) << RED << "Method not allowed for " << method << " request: " << RESET << req.getPath() << std::endl;
			sendErrorResponse(405, req);
			return "";
		}
	}
	if (loc->uploadDirPath.empty()) {
		std::cerr << getTimeStamp(_fd) << RED << "Upload directory not set for " << method << " request: " << RESET << req.getPath() << std::endl;
		sendErrorResponse(403, req);
		return "";
	}
	std::string fullPath = matchAndAppendPath(loc->rootLoc, loc->uploadDirPath);
	fullPath = matchAndAppendPath(fullPath, req.getPath());
    return fullPath;
}

int Client::handlePostRequest(Request& req) {
	locationLevel* loc = NULL;
    if (!matchLocation(req.getPath(), req.getConf(), loc)) {
		std::cerr << getTimeStamp(_fd) << RED  << "Location not found for POST request: " 
				  << RESET << req.getPath() << std::endl;
		sendErrorResponse(404, req);
		return 1;
	}
    std::string fullPath = getLocationPath(req, "POST");
    if (fullPath.empty())
        return 1;
    if (isCGIScript(req.getPath())) {
        CGIHandler* cgi = new CGIHandler(*this);
        std::string cgiPath = matchAndAppendPath(_server->getWebRoot(req, *loc), req.getPath());
        cgi->setPath(cgiPath);

        int result = cgi->executeCGI(req);
        
        if (result != 0) {
            delete cgi;
        }
        
        return result;
    }
    
    if (req.getContentType().find("multipart/form-data") != std::string::npos)
        return handleMultipartPost(req);
    
    if (req.getPath().find("../") != std::string::npos) {
        sendErrorResponse(403, req);
        return 1;
    }
    
    std::string contentToWrite;
    
    if (isChunkedRequest(req)) {
        std::cout << getTimeStamp(_fd) << BLUE << "Processing chunked request" << RESET << std::endl;
        contentToWrite = decodeChunkedBody(req.getBody());
        
        if (contentToWrite.empty()) {
            std::cerr << getTimeStamp(_fd) << RED  << "Failed to decode chunked data" << RESET << std::endl;
            sendErrorResponse(400, req);
            return 1;
        }
    } else {
        contentToWrite = req.getBody();
		std::string fileName;
        if (contentToWrite.empty() && !req.getQuery().empty())
			doQueryStuff(req.getQuery(), fileName, contentToWrite);
        else
			doQueryStuff(req.getBody(), fileName, contentToWrite);
		fullPath = matchAndAppendPath(fullPath, fileName);
    }
    int fd = open(fullPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        std::cerr << getTimeStamp(_fd) << RED << "Failed to open file for writing: " << RESET << fullPath << std::endl;
        sendErrorResponse(500, req);
        return 1;
    }
    
    std::cout << getTimeStamp(_fd) << BLUE << "Writing to file: " << RESET << fullPath << std::endl;
    
    ssize_t bytesWritten = write(fd, contentToWrite.c_str(), contentToWrite.length());
    
	if (!checkReturn(_fd, bytesWritten, "write()", "Failed to write to file")) {
        sendErrorResponse(500, req);
		close(fd);
        return 1;
    }
    std::string responseBody = "File uploaded successfully. Wrote " + tostring(bytesWritten) + " bytes.";
    _connect = "keep-alive";
    ssize_t responseResult = sendResponse(req, "keep-alive", responseBody);
    
    if (responseResult < 0) {
        std::cerr << getTimeStamp(_fd) << RED  << "Failed to send response" << RESET << std::endl;
        return 1;
    }
    
    std::cout << getTimeStamp(_fd) << GREEN  << "Uploaded file: " << RESET << fullPath 
              << " (" << bytesWritten << " bytes written)" << std::endl;
    close(fd);
    return 0;
}

int Client::handleDeleteRequest(Request& req) {
	std::string fullPath = getLocationPath(req, "DELETE");
	if (fullPath.empty())
		return 1;
    if (isCGIScript(req.getPath())) {
        CGIHandler* cgi = new CGIHandler(*this);
        cgi->setPath(fullPath);
        std::cout << MAGENTA << "HERE 3: " << fullPath << RESET << std::endl;
        
        int result = cgi->executeCGI(req);
        
        if (result != 0) {
            delete cgi;
        }
        
        return result;
    }

    size_t end = fullPath.find_last_not_of(" \t\r\n");
    if (end != std::string::npos)
        fullPath = fullPath.substr(0, end + 1);

    if (unlink(fullPath.c_str()) != 0) {
		if (errno == ENOENT) {
			std::cerr << getTimeStamp(_fd) << RED  << "File not found for deletion: " << RESET << fullPath << std::endl;
			sendErrorResponse(404, req);
			return 1;
		} else if (errno == EACCES || errno == EPERM) {
			std::cerr << getTimeStamp(_fd) << RED  << "Permission denied for deletion: " << RESET << fullPath << std::endl;
			sendErrorResponse(403, req);
			return 1;
		} else {
			std::cerr << getTimeStamp(_fd) << RED  << "Error deleting file: " << RESET << fullPath
					  << " - " << strerror(errno) << std::endl;
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
        return 1;
    }
    
    if (!saveFile(req, filename, fileContent)) {
        sendErrorResponse(500, req);
        return 1;
    }
    std::cout << getTimeStamp(_fd) << GREEN  << "Received: " + parser.getFilename() << RESET << std::endl;
    std::string successMsg = "Successfully uploaded file:" + filename;
    sendResponse(req, "close", successMsg);
    std::cout << getTimeStamp(_fd) << GREEN  << "File transfer ended" << RESET << std::endl;    
    return 0;
}

bool Client::ensureUploadDirectory(Request& req) {
    struct stat st;
	std::string uploadDir = _server->getUploadDir(*this, req);
    if (stat(uploadDir.c_str(), &st) != 0) {
        if (mkdir(uploadDir.c_str(), 0755) != 0) {
            std::cerr << getTimeStamp(_fd) << RED << "Error: Failed to create upload directory" << RESET << std::endl;
            return false;
        }
    }
    return true;
}

bool Client::saveFile(Request& req, const std::string& filename, const std::string& content) {
	std::string fullPath = matchAndAppendPath(_server->getUploadDir(*this, req), filename);
	if (fullPath.empty())
		return false;
	if (!ensureUploadDirectory(req)) {
		std::cerr << getTimeStamp(_fd) << RED << "Error: Failed to ensure upload directory exists" << RESET << std::endl;
		return false;
	}
    
    int fd = open(fullPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        std::cerr << getTimeStamp(_fd) << RED << "Error: Failed to open file for writing: " << RESET << fullPath << std::endl;
        return false;
    }
    
    ssize_t bytesWritten = write(fd, content.c_str(), content.length());
    if (!checkReturn(_fd, bytesWritten, "write()", "Failed to write to file")) {
        sendErrorResponse(500, req);
		close(fd);
        return false;
    }
	close(fd);
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
    
    std::string body = "<!DOCTYPE html><html><head><title>" + statusText + "</title></head>";
    body += "<body><h1>" + statusText + "</h1>";
    body += "<p>The document has moved <a href=\"" + location + "\">here</a>.</p></body></html>";
    
    std::string response = "HTTP/1.1 " + tostring(statusCode) + " " + statusText + "\r\n";
    response += "Location: " + location + "\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + tostring(body.length()) + "\r\n";
    response += "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n";
    response += "Pragma: no-cache\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += body;
    _connect = "close";
	_webserv->addSendBuf(_fd, response);
	_webserv->setEpollEvents(_fd, EPOLLOUT);
    std::cout << getTimeStamp(_fd) << BLUE << "Sent redirect response: " << RESET
              << statusCode << " " << statusText << " to " << location << std::endl;
}

ssize_t Client::sendResponse(Request req, std::string connect, std::string body) {
	_connect = connect;
    if (_fd <= 0) {
        std::cerr << getTimeStamp(_fd) << RED << "Invalid fd in sendResponse" << RESET << std::endl;
        return -1;
    }
    std::string response = "HTTP/1.1 200 OK\r\n";
    
    std::map<std::string, std::string> headers = req.getHeaders();
    bool isChunked = false;
    std::map<std::string, std::string>::iterator it = headers.find("Transfer-Encoding");
    if (it != headers.end() && it->second.find("chunked") != std::string::npos)
        isChunked = true;
    
    response += "Content-Type: " + req.getContentType() + "\r\n";
    
    std::string content = body;
    
    if (req.getMethod() == "POST" && content.empty())
        content = "Upload successful";
    
    if (!isChunked)
        response += "Content-Length: " + tostring(content.length()) + "\r\n";
	else
        response += "Transfer-Encoding: chunked\r\n";
    
    response += "Server: WebServ/1.0\r\n";
    response += "Connection: " + connect + "\r\n";
    response += "Access-Control-Allow-Origin: *\r\n";
    response += "\r\n";
    
    if (_fd < 0) {
        std::cerr << getTimeStamp(_fd) << RED  << "FD became invalid before send" << RESET << std::endl;
        return -1;
    }
    
	_webserv->addSendBuf(_fd, response);
    
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
				std::string all = hexStream.str() + "\r\n";
				all += content.c_str() + offset;
				all += "\r\n";
				_webserv->addSendBuf(_fd, all);
                offset += currentChunkSize;
                remaining -= currentChunkSize;
            }
            
			std::string s2 = "0\r\n\r\n";
			_webserv->addSendBuf(_fd, s2);
			_webserv->setEpollEvents(_fd, EPOLLOUT);
            std::cout << getTimeStamp(_fd) << GREEN  << "Sent chunked body " << RESET << "(" << content.length() << " bytes)\n";
            return content.length();
        } else {
			_webserv->addSendBuf(_fd, content);
			_webserv->setEpollEvents(_fd, EPOLLOUT);
            return content.length();
        }
    } else {
		_webserv->setEpollEvents(_fd, EPOLLOUT);
        std::cout << getTimeStamp(_fd) << GREEN  << "Response sent (headers only)" << RESET << std::endl;
        return 0;
    }
}

void Client::sendErrorResponse(int statusCode, Request& req) {
    std::string body;
    std::string statusText = getStatusMessage(statusCode);
    
	resolveErrorResponse(statusCode, statusText, body, req);    
  
    std::string response = "HTTP/1.1 " + tostring(statusCode) + " " + statusText + "\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + tostring(body.size()) + "\r\n";
    response += "Server: WebServ/1.0\r\n";
    response += "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n";
    response += "Pragma: no-cache\r\n";
    if (statusCode == 413) {
		_connect = "keep-alive";
        response += "Connection: keep-alive\r\n";
	} else {
		_connect = "close";
        response += "Connection: close\r\n";
	}
    response += "\r\n";
    response += body;
    _webserv->addSendBuf(_fd, response);
	_webserv->setEpollEvents(_fd, EPOLLOUT);
    std::cerr << getTimeStamp(_fd) << RED  << "Error sent: " << statusCode << RESET << std::endl;
}

bool Client::isChunkedRequest(const Request& req) {
    std::map<std::string, std::string> headers = const_cast<Request&>(req).getHeaders();
    std::map<std::string, std::string>::iterator it = headers.find("Transfer-Encoding");
    if (it != headers.end() && it->second.find("chunked") != std::string::npos)
        return true;
    return false;
}

std::string Client::decodeChunkedBody(const std::string& chunkedData) {
    std::string decodedBody;
    size_t pos = 0;

    
    while (pos < chunkedData.length()) {
        size_t crlfPos = chunkedData.find("\r\n", pos);
        size_t lineEndLength = 2;
        
        if (crlfPos == std::string::npos) {
            crlfPos = chunkedData.find("\n", pos);
            lineEndLength = 1;
            if (crlfPos == std::string::npos) {
                std::cerr << getTimeStamp(_fd) << RED  << "Malformed chunked data: no CRLF after chunk size" << RESET << std::endl;
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
        
        if (chunkSize == 0)
            break;
        
        pos = crlfPos + lineEndLength;
        
        if (pos + chunkSize > chunkedData.length())
            break;
        
        std::string chunkData = chunkedData.substr(pos, chunkSize);
        decodedBody += chunkData;
        
        pos += chunkSize;
        
        if (pos < chunkedData.length() && chunkedData[pos] == '\r') {
            pos++;
        }
        if (pos < chunkedData.length() && chunkedData[pos] == '\n') {
            pos++;
        }
    }
    
    std::cout << getTimeStamp(_fd) << GREEN  << "Total decoded body: " << RESET << "\"" 
              << decodedBody << "\" (" << decodedBody.length() << " bytes)" << std::endl;
    
    return decodedBody;
}