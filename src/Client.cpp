#include "../include/Client.hpp"
#include "../include/Server.hpp"
#include "../include/Webserv.hpp"

Client::Client() : _webserv(NULL), _server(NULL) {
}

Client::Client(Server& serv) {
    setWebserv(&serv.getWebServ());
    setServer(&serv);
	setConfig(Config(serv.getConfigClass()).getConfig());
    _cgi.setClient(*this);
    _cgi.setServer(*_server);
    _cgi.setConfig(serv.getConfigClass());
    _cgi.setConfig(serv.getConfigClass());
    _cgi.setCGIBin(&_config);
    setAutoIndex();
}

Client::~Client() {

}

struct sockaddr_in  &Client::getAddr() {
    return _addr;
}

socklen_t   &Client::getAddrLen() {
    return _addrLen;
}

int     &Client::getFd() {
    return _fd;
}

unsigned char &Client::getIP() {
    return *_ip;
}

char    &Client::getBuffer() {
    return *_buffer;
}

void Client::setWebserv(Webserv *webserv) {
    _webserv = webserv;
}

void Client::setServer(Server *server) {
    _server = server;
}

Server &Client::getServer() {
    return *_server;
}

Webserv &Client::getWebserv() {
	return *_webserv;
}

void    Client::setConfig(serverLevel config) {
    _config = config;
}

void    Client::setAutoIndex() {
    std::map<std::string, locationLevel>::iterator it = _config.locations.begin();
    if (it != _config.locations.end() && it->second.autoindex == true) {
        _autoindex = true;
    } else {
        _autoindex = false;
    }
}

int Client::acceptConnection(int serverFd) {
    _addrLen = sizeof(_addr);
    _fd = accept(serverFd, (struct sockaddr *)&_addr, &_addrLen);
    if (_fd < 0) {
        _webserv->ft_error("Accept failed");
        return 1;
    }
    
    // Set up server-specific configs
    Config *temp = new Config(_server->getConfigClass());
    setConfig(temp->getConfig());
    delete temp;
    
    _cgi.setServer(*_server);
    
    setAutoIndex();
    
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
    // Larger buffer for multipart uploads
    char buffer[1024 * 1024];
    memset(buffer, 0, sizeof(buffer) - 1);
    
    // Receive data
    int bytesRead = recv(_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead > 0) {
        // Append to request buffer
        _requestBuffer.append(buffer, bytesRead);

        if (_requestBuffer.length() > _config.requestLimit) {
            std::cerr << RED << _webserv->getTimeStamp() << "Buffer size exceeded maximum limit: Error 413" << RESET << std::endl;
            sendErrorResponse(413);
            _requestBuffer.clear();
            return 1;
        }

        std::cout << BLUE << _webserv->getTimeStamp() 
                  << "Received " << bytesRead << " bytes from " << _fd 
                  << ", Total buffer: " << _requestBuffer.length() << " bytes" << RESET << "\n";

		Request req(_requestBuffer);
        if (req.getContentLength() > _config.requestLimit) {
            std::cerr << RED << "Content-Length too large" << RESET << std::endl;
            sendErrorResponse(413);
            _requestBuffer.clear();
            return 1;
        }
	
		if (req.getMethod() == "BAD") {
			std::cout << RED << _webserv->getTimeStamp() 
					<< "Bad request format" << RESET << std::endl;
			sendErrorResponse(400);
			_requestBuffer.clear();
			return 1;
		}
		// Handle multipart uploads separately
		if (req.getContentType().find("multipart/form-data") != std::string::npos) {
			int result = handleMultipartPost(req);
			
			if (result != -1) {
				return result;
			}
			return 0;
		}

		// Create a copy of the buffer for processing
		std::string tempBuffer = _requestBuffer;

		// Try processing the request
		int processResult = processRequest(const_cast<char*>(tempBuffer.c_str()));
		
		// If fully processed, clear the buffer
		if (processResult != -1) {
			_requestBuffer.clear();
		}

		return processResult;
    } 
    else if (bytesRead == 0) {
        std::cout << RED << _webserv->getTimeStamp() 
                  << "Client disconnected: " << _fd << RESET << "\n";
        return 1;
    }
    else { // Error occurred TODO: Possible not allowed?
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        } else {
            return (_webserv->ft_error("recv() failed"), 1);
        }
    }

}

Request Client::parseRequest(char* buffer) {
    std::string input;
    if (buffer) {
        size_t len = strlen(buffer);
        input.assign(buffer, len);
    }

    Request req(input);

    return req;
}

int Client::processRequest(char *buffer) {
    Request req = parseRequest(buffer);
    if (req.getMethod() == "BAD") {
        sendErrorResponse(400);
        return 1;
    }

    // Debug print
    std::cout << BLUE << _webserv->getTimeStamp();
    std::cout << "Parsed Request: " << RESET << req.getMethod() << " " << req.getPath() << " " << req.getVersion() << "\n";

    if (req.getMethod() == "GET") {
        return handleGetRequest(req);
    } else if (req.getMethod() == "POST") {
        return handlePostRequest(req);
    } else if (req.getMethod() == "DELETE") {
        return handleDeleteRequest(req);
    } else {
        sendErrorResponse(405);
        return 1;
    }
}

int Client::handleGetRequest(Request& req) {
	std::string requestPath = req.getPath();
	locationLevel loc;
	if (!matchLocation(req.getReqPath(), _server->getConfigClass().getConfig(), loc)) {
		std::cout << RED << _webserv->getTimeStamp() << "Location not found: " << RESET << req.getReqPath() << std::endl;
		sendErrorResponse(404);
		return 1;
	}
    // Check for path traversal attempts
    if (requestPath.find("../") != std::string::npos) {
        sendErrorResponse(403);
        return 1;
    }
    
    // Separate file browser functionality from regular web serving
    if (_autoindex == true && isFileBrowserRequest(requestPath)) {
        return handleFileBrowserRequest(req, requestPath);
    } else {
        return handleRegularRequest(req, requestPath);
    }
}

bool Client::isFileBrowserRequest(const std::string& path) {
    return (path.length() >= 6 && path.substr(0, 6) == "/root/") || 
           (path == "/root");
}

int Client::handleFileBrowserRequest(Request& req, const std::string& requestPath) {
    // Remove "/root" prefix to get the actual path relative to web root
    std::string actualPath;
    if (requestPath == "/root" || requestPath == "/root/") {//TODO: check
        actualPath = "/";
    } else if (requestPath.find("/root/") == 0) {
        actualPath = requestPath.substr(5); // Remove "/root"
        if (actualPath.empty()) actualPath = "/";
    } else {
        // Sub-directory or file within root/
        std::string actualPath = requestPath.substr(5); // Remove the /root prefix
        std::string actualFullPath = _server->getWebRoot() + actualPath;
        
        struct stat fileStat;
        if (stat(actualFullPath.c_str(), &fileStat) != 0) {
            std::cout << RED << _webserv->getTimeStamp() << "File not found: " << RESET << actualFullPath  << "\n";
            sendErrorResponse(404);
            return 1;
        }
        
        if (S_ISDIR(fileStat.st_mode)) {
            // If it's a directory, show directory listing
            return createDirList(actualFullPath, actualPath);
        } else if (S_ISREG(fileStat.st_mode)) {
            // If it's a regular file, serve it
            if (buildBody(req, actualFullPath) == 1) {
                return 1;
            }
            
            // Set proper content type and serve the file
            req.setContentType(req.getMimeType(actualFullPath));
            sendResponse(req, "keep-alive", req.getBody());
            std::cout << GREEN << _webserv->getTimeStamp() << "Successfully served file from browser: " 
                    << RESET << actualFullPath << std::endl;
            return 0;
        } else {
            // Not a regular file or directory
            std::cout << _webserv->getTimeStamp() << "Not a regular file or directory: " << actualFullPath << std::endl;
            sendErrorResponse(403);
            return 1;
        }
    }
	return 0;
}

// Fix for handleRegularRequest in Client.cpp
int Client::handleRegularRequest(Request& req, const std::string& requestPath) {
    std::string reqPath = req.getPath();//TODO: check
    if (reqPath == "/" || reqPath.empty()) {
        reqPath = "/index.html";
    }

    size_t end = reqPath.find_last_not_of(" \t\r\n");
    if (end != std::string::npos) {
        reqPath = reqPath.substr(0, end + 1);
    }
	std::string fullPath = _server->getWebRoot() + requestPath;

    // Check if path contains "root" and autoindex is disabled
    if (fullPath.find("root") != std::string::npos && _autoindex == false) {
        std::cout << RED << "Access to directory browser is forbidden when autoindex is off\n" << RESET;
        sendErrorResponse(403);
        return 1;
    }

    if (handleRedirect(req) == 0) {
        return 1;
    }

    // Check if it's a CGI script
    if (_cgi.isCGIScript(reqPath)) {
        return _cgi.executeCGI(*this, req, fullPath);
    }

    std::cout << _webserv->getTimeStamp() << "Handling GET request for path: " << req.getPath() << std::endl;
    std::cout << _webserv->getTimeStamp() << "Full file path: " << fullPath << "\n";

    // Check if file exists and is regular file
    struct stat fileStat;
    if (stat(fullPath.c_str(), &fileStat) != 0) {
        std::cout << RED << _webserv->getTimeStamp() << "File not found: " << RESET << fullPath << "\n";
        sendErrorResponse(404);
        return 1;
    }
    
    // Check if it's a directory
    if (S_ISDIR(fileStat.st_mode)) {
        return viewDirectory(fullPath, requestPath);
    } else if (!S_ISREG(fileStat.st_mode)) {
        // Not a regular file or directory
        std::cout << _webserv->getTimeStamp() << "Not a regular file: " << fullPath << std::endl;
        sendErrorResponse(403);
        return 1;
    }
    
    // Build response body from file
    if (buildBody(req, fullPath) == 1) {
        return 1;
    }

    // Set content type based on file extension
    std::string contentType = req.getMimeType(fullPath);
    if (fullPath.find(".html") != std::string::npos || 
        reqPath == "/" || 
        reqPath == "/index.html") {
        contentType = "text/html";
    }

    req.setContentType(contentType);

    // Send response
    sendResponse(req, "keep-alive", req.getBody());
    std::cout << GREEN << _webserv->getTimeStamp() << "Successfully sent file: " << RESET << fullPath << std::endl;
    return 0;
}

int Client::buildBody(Request &req, std::string fullPath) {
    // Open file
    int fd = open(fullPath.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cout << _webserv->getTimeStamp() << "Failed to open file: " << fullPath << std::endl;
        sendErrorResponse(500);
        return 1;
    }
    
    // Get file size
    struct stat fileStat;
    if (fstat(fd, &fileStat) < 0) {
        close(fd);
        sendErrorResponse(500);
        return 1;
    }
    
    // Allocate a buffer for the entire file
    std::vector<char> buffer(fileStat.st_size);
    
    // Read the entire file at once
    ssize_t bytesRead = read(fd, buffer.data(), fileStat.st_size);
    close(fd);
    
    // Set the body content
    // req.setBody(fileContent);// + "\r\n");

    // Check for read errors
    if (bytesRead < 0) {
        std::cout << "Error reading file: " << fullPath << std::endl;
        sendErrorResponse(500);
        return 1;
    }
    
    // Set the body directly from the buffer without null termination
    std::string fileContent(buffer.data(), bytesRead);
    req.setBody(fileContent);
    
    return 0;
}

int Client::viewDirectory(std::string fullPath, std::string requestPath) {
    // Find the location configuration that matches the request path
    std::string bestMatch = "";
        
    // Iterate through all locations to find the best match
    std::map<std::string, locationLevel>::const_iterator it;
    for (it = _config.locations.begin(); it != _config.locations.end(); ++it) {
        std::string locationPath = it->first; // This is the URL path like "/images"
        
        // Check if this location is a prefix of the requested path
        if (requestPath.find(locationPath) == 0) {
            // If this is a longer match than what we have, use it
            if (locationPath.length() > bestMatch.length()) {
                bestMatch = locationPath;
            }
        }
    }
    
    // If we found a matching location
    if (!bestMatch.empty() && _config.locations.find(bestMatch) != _config.locations.end()) {
        // Check if autoindex is enabled
        if (_autoindex == true) {
            return createDirList(fullPath, requestPath);
        } 
        // If autoindex is disabled, try to serve index.html in the requested directory
        else {
            // Fix: Use the actual requested directory's index.html
            std::string indexPath = fullPath + "/index.html";
            struct stat indexStat;
            if (stat(indexPath.c_str(), &indexStat) == 0 && S_ISREG(indexStat.st_mode)) {
                // index.html exists, serve it
                Request req;
                req.setPath(indexPath);
                if (buildBody(req, indexPath) == 1) {
                    return 1;
                }
                
                // Set content type and send the file
                req.setContentType("text/html");
                sendResponse(req, "keep-alive", req.getBody());
                std::cout << GREEN << _webserv->getTimeStamp() << "Successfully served index file: " 
                        << RESET << indexPath << std::endl;
                return 0;
            } else {
                // No autoindex and no index.html, return 403
                std::cout << _webserv->getTimeStamp() << "Directory listing not allowed and no index.html: " << fullPath << std::endl;
                sendErrorResponse(403);
                return 1;
            }
        }
    } 
    // No matching location found, check if there's a default location
    else if (_config.locations.find("/") != _config.locations.end()) {
        if (_config.locations.find("/")->second.autoindex) {
            return createDirList(fullPath, requestPath);
        } 
        // If autoindex is disabled, try to serve index.html
        else {
            // Fix: Use the actual requested directory's index.html
            std::string indexPath = fullPath + "/index.html";
            struct stat indexStat;
            if (stat(indexPath.c_str(), &indexStat) == 0 && S_ISREG(indexStat.st_mode)) {
                // index.html exists, serve it
                Request req;
                req.setPath(indexPath);
                if (buildBody(req, indexPath) == 1) {
                    return 1;
                }
                
                // Set content type and send the file
                req.setContentType("text/html");
                sendResponse(req, "keep-alive", req.getBody());
                std::cout << GREEN << _webserv->getTimeStamp() << "Successfully served index file: " 
                        << RESET << indexPath << std::endl;
                return 0;
            } else {
                // No autoindex and no index.html, return 403
                std::cout << _webserv->getTimeStamp() << "Directory listing not allowed and no index.html: " << fullPath << std::endl;
                sendErrorResponse(403);
                return 1;
            }
        }
    }
    // No matching location and no default, return 403
    else {
        std::cout << _webserv->getTimeStamp() << "No matching location for: " << requestPath << std::endl;
        sendErrorResponse(403);
        return 1;
    }
    return 0;
}

int Client::createDirList(std::string fullPath, std::string requestPath) {
    // Generate directory listing without checking for index.html
    std::string dirListing = showDir(fullPath, requestPath);
    if (dirListing.empty()) {
        sendErrorResponse(403);
        return 1;
    }
    
    // Send directory listing as response
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
    
    // Add parent directory link if not at root
    if (requestUri != "/") {
        // Calculate parent directory path
        std::string parentUri = requestUri;
        
        // Remove trailing slash if present
        if (parentUri[parentUri.length() - 1] == '/') {
            parentUri = parentUri.substr(0, parentUri.length() - 1);
        }
        
        // Find the last slash
        size_t lastSlash = parentUri.find_last_of('/');
        if (lastSlash != std::string::npos) {
            // Get parent directory path
            parentUri = parentUri.substr(0, lastSlash + 1);
        } else {
            parentUri = "/";
        }
        
        // Fix: Create parent directory link WITHOUT /root/ prefix
        html += "        <li><a href=\"" + parentUri + "\">Parent Directory</a></li>\n";
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        
        // Skip . and .. entries
        if (name == "." || name == "..") {
            continue;
        }
        
        // Check if it's a directory
        std::string entryPath = dirPath + "/" + name;
        struct stat entryStat;
        if (stat(entryPath.c_str(), &entryStat) == 0) {
            if (S_ISDIR(entryStat.st_mode)) {
                name += "/"; // Add trailing slash for directories
            }
        }
        
        // Fix: Create link WITHOUT /root/ prefix to avoid path doubling
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
	locationLevel loc;
	if (!matchUploadLocation(req.getReqPath(), _config, loc)) {
		std::cout << "Location not found for " << method << " request: " << req.getReqPath() << std::endl;
		sendErrorResponse(403);
		return "";
	}
	for (size_t i = 0; i < loc.methods.size(); i++) {
		if (loc.methods[i] == method) {
			break;
		}
		if (i == loc.methods.size() - 1) {
			std::cout << "Method not allowed for " << method << "request: " << req.getReqPath() << std::endl;
			sendErrorResponse(405);
			return "";
		}
	}
	if (loc.uploadDirPath.empty()) {
		std::cout << "Upload directory not set for " << method << "request: " << req.getReqPath() << std::endl;
		sendErrorResponse(403);
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
	std::string fullPath = getLocationPath(req, "POST");
	if (fullPath.empty())
		return 1;
	
    if (_cgi.isCGIScript(req.getPath())) {
		return _cgi.executeCGI(*this, req, fullPath);
    }
    // Check if this is a multipart/form-data request and handle it separately
    if (req.getContentType().find("multipart/form-data") != std::string::npos) {
		return handleMultipartPost(req);
    }
	
    if (req.getPath().find("../") != std::string::npos) {
		sendErrorResponse(403);
        return 1;
    }
	
	// Fix: Use the actual file name from the request
    int fd = open(fullPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        std::cout << _webserv->getTimeStamp() << "Failed to open file for writing: " << fullPath << std::endl;
        sendErrorResponse(500);
        return 1;
    }

    std::cout << _webserv->getTimeStamp() << "Writing to file: " << fullPath << "\n";
    std::cout << _webserv->getTimeStamp() << "Content to write: " << req.getQuery() << "\n";

    ssize_t bytesWritten = write(fd, req.getQuery().c_str(), req.getQuery().length());
    close(fd);

    if (bytesWritten < 0) {
        std::cout << "Failed to write to file" << "\n";
        sendErrorResponse(500);
        return 1;
    }

    sendResponse(req, "keep-alive", "");

    std::cout << _webserv->getTimeStamp() << "Successfully uploaded file: " << fullPath << std::endl;
    return 0;
}

int Client::handleDeleteRequest(Request& req) {
	std::string fullPath = getLocationPath(req, "DELETE");
	if (fullPath.empty())
		return 1;
    if (_cgi.isCGIScript(req.getPath())) {
        return _cgi.executeCGI(*this, req, fullPath);
    }

    size_t end = fullPath.find_last_not_of(" \t\r\n");
    if (end != std::string::npos) {
        fullPath = fullPath.substr(0, end + 1);
    }

    if (unlink(fullPath.c_str()) != 0) {
        sendErrorResponse(403);
        return 1;
    }

    sendResponse(req, "keep-alive", "");
    return 0;
}

int Client::handleMultipartPost(Request& req) {
    std::string boundary = req.getBoundary();
    
    if (boundary.empty()) {
        sendErrorResponse(400);
        return 1;
    }
    
    Multipart parser(_requestBuffer, boundary);
    
    if (!parser.parse()) {
        sendErrorResponse(400);
        return 1;
    }
    
    std::string filename = parser.getFilename();
    if (filename.empty()) {
        sendErrorResponse(400);
        return 1;
    }

    std::string fileContent = parser.getFileContent();
    if (fileContent.empty() && !parser.isComplete()) {
        return -1;
    }
    
    if (!saveFile(req, filename, fileContent)) {
        sendErrorResponse(500);
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
	const char *path = uploadDir.c_str();
    if (stat(path, &st) != 0) {
		std::cout << "Creating upload directory: " << path << std::endl;
        if (mkdir(path, 0755) != 0) {
            std::cout << "Error: Failed to create upload directory" << std::endl;
            return false;
        }
    }
    return true;
}

bool Client::saveFile(Request& req, const std::string& filename, const std::string& content) {
    std::string fullPath = _server->getUploadDir(*this, req) + filename;
    
    int fd = open(fullPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        std::cout << "Error: Failed to open file for writing: " << fullPath << std::endl;
        std::cout << "Error details: " << strerror(errno) << std::endl;
        return false;
    }
    
    ssize_t bytesWritten = write(fd, content.c_str(), content.length());
    close(fd);
    
    if (bytesWritten < 0) {
        std::cout << "Error: Failed to write to file: " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

int    Client::handleRedirect(Request req) {
    std::string path = req.getPath().substr(1);
    std::map<std::string, locationLevel>::iterator it = _config.locations.begin();
    for ( ; it != _config.locations.end() ; it++) {
        if (it->first == path) {
            sendRedirect(301, it->second.redirectionHTTP);
            return 0;
        }
    }
    return 1;
}

void Client::sendRedirect(int statusCode, const std::string& location) {
    std::string statusText;
    if (statusCode == 301) {
        statusText = "Moved Permanently";
    } else {
        statusText = "Found";
    }
    
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

void Client::findContentType(Request& req) {
    std::string raw(_buffer);
    size_t start = raw.find("Content-Type: ");
    
    if (start == std::string::npos) {
        // Content-Type header not found
        if (raw.find("POST") == 0) {
            std::cout << "Warning: POST request without Content-Type header" << std::endl;
            req.setContentType("application/octet-stream");
        }
        return;
    }
    
    // Extract the full Content-Type header value
    start += 14; // Skip "Content-Type: "
    size_t end = raw.find("\r\n", start);
    if (end == std::string::npos) {
        end = raw.find("\n", start);
        if (end == std::string::npos) {
            end = raw.length();
        }
    }
    
    // Get the complete Content-Type value
    std::string contentType = raw.substr(start, end - start);
    req.setContentType(contentType); // Store the full Content-Type
    
    //std::cout << "Found Content-Type: [" << contentType << "]" << std::endl;
    
    // Check for boundary parameter
    size_t boundaryPos = contentType.find("; boundary=");
    if (boundaryPos != std::string::npos) {
        std::string boundary = contentType.substr(boundaryPos + 11); // +11 for "; boundary="
        
        // Clean up boundary if needed
        size_t quotePos = boundary.find("\"");
        if (quotePos != std::string::npos) {
            boundary = boundary.substr(0, quotePos);
        }
        
        //std::cout << "Boundary: [" << boundary << "]" << std::endl;
        
        if (boundary.empty()) {
            std::cout << "Warning: Empty boundary found" << std::endl;
        }
        
        req.setBoundary(boundary);
    }
}

ssize_t Client::sendResponse(Request req, std::string connect, std::string body) {
    // Create HTTP response
	std::string response = "HTTP/1.1 200 OK\r\n";
    
    // Get proper content type based on file extension
    std::string contentType;
    
    if (req.getPath().empty() || req.getPath() == "/") {
        contentType = "text/html";
    } else {
        contentType = req.getMimeType(req.getPath());
    }
    
    // Check for chunked transfer encoding
    std::map<std::string, std::string> headers = req.getHeaders();
    bool isChunked = false;
    std::map<std::string, std::string>::iterator it = headers.find("Transfer-Encoding");
    if (it != headers.end() && it->second.find("chunked") != std::string::npos) {
        isChunked = true;
    }
    
    // Set Content-Type header
    response += "Content-Type: " + contentType + "\r\n";
    
    // Choose which content to use for Content-Length
    std::string content = req.getBody();
    if (req.getMethod() == "POST" && body.empty()) {
        content = req.getQuery();
    } else if (!body.empty()) {
        content = body;
    }
    
    // Add remaining headers
    if (!isChunked) {
        response += "Content-Length: " + std::string(tostring(content.length())) + "\r\n";
    } else {
        response += "Transfer-Encoding: chunked\r\n";
    }
    
    response += "Server: WebServ/1.0\r\n";
    response += "Connection: " + connect + "\r\n";
    response += "Access-Control-Allow-Origin: *\r\n\r\n";
    
    // Send headers
    send(_fd, response.c_str(), response.length(), 0);
    
    // Send body content if there is any
    if (!content.empty()) {
        if (isChunked) {
            // Format body as chunked if needed
            const size_t chunkSize = 4096;
            size_t remaining = content.length();
            size_t offset = 0;
            
            while (remaining > 0) {
                size_t currentChunkSize = (remaining < chunkSize) ? remaining : chunkSize;
                
                // Add chunk header (size in hex)
                std::stringstream hexStream;
                hexStream << std::hex << currentChunkSize;
                std::string chunkHeader = hexStream.str() + "\r\n";
                send(_fd, chunkHeader.c_str(), chunkHeader.length(), 0);
                
                // Add chunk data
                send(_fd, content.c_str() + offset, currentChunkSize, 0);
                send(_fd, "\r\n", 2, 0);
                
                offset += currentChunkSize;
                remaining -= currentChunkSize;
            }
            
            // Add terminating chunk
            send(_fd, "0\r\n\r\n", 5, 0);
            
            return content.length(); // Return original content length
        } else {
            return send(_fd, content.c_str(), content.length(), 0);
        }
    }
    
    return 0;
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

void Client::sendErrorResponse(int statusCode) {
    std::string body;
    std::string statusText = getStatusMessage(statusCode);
    
    resolveErrorResponse(statusCode, *_server, statusText, body);    
    // Construct the HTTP response
    std::string response = "HTTP/1.1 " + tostring(statusCode) + " " + statusText + "\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + tostring(body.size()) + "\r\n";
    response += "Server: WebServ/1.0\r\n";
    
    // Important: Set Cache-Control to prevent browsers from caching error responses
    response += "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n";
    response += "Pragma: no-cache\r\n";
    
    // Close the connection after error responses
    response += "Connection: close\r\n";
    response += "\r\n";
    response += body;
    response += "\r\n";

    char discard_buffer[1024];
    while (recv(_fd, discard_buffer, sizeof(discard_buffer), MSG_DONTWAIT) > 0) {
        // Just discard the data
    }
    if (!send_all(_fd, response)) {
        std::cerr << "Failed to send error response" << std::endl;
    }
    std::cerr << RED << _webserv->getTimeStamp() << "Error sent: " << statusCode << RESET << "\n";
}

