#include "../include/Client.hpp"
#include "../include/Server.hpp"
#include "../include/Webserv.hpp"

Client::Client() : _webserv(NULL), _server(NULL) {

}

Client::Client(Webserv &other) {
    setWebserv(&other);
    setServer(&other.getServer());

    _cgi.setClient(*this);
    _cgi.setServer(*_server);
    _cgi.setConfig(_webserv->getConfig());
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

Webserv &Client::getWebserv() {
	return *_webserv;
}

Server &Client::getServer() {
	return *_server;
}

int Client::acceptConnection() {
    _addrLen = sizeof(_addr);
    _fd = accept(_server->getFd(), (struct sockaddr *)&_addr, &_addrLen);
    if (_fd < 0) {
        _webserv->ft_error("Accept failed");
        return 1;
    }
    return 0;
}

void Client::displayConnection() {
    unsigned char *ip = (unsigned char *)&_addr.sin_addr.s_addr;
    std::cout << BLUE << _webserv->getTimeStamp() << "New connection from "
                << (int)ip[0] << "."
                << (int)ip[1] << "."
                << (int)ip[2] << "."
                << (int)ip[3]
                << ":" << ntohs(_addr.sin_port) << RESET << "\n";
}

int Client::recieveData() {
    // Larger buffer for multipart uploads
    char buffer[1024 * 1024];
    memset(buffer, 0, sizeof(buffer));
    
    // Receive data
    int bytesRead = recv(_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead > 0) {
        // Append to request buffer
        _requestBuffer.append(buffer, bytesRead);

        if (_requestBuffer.length() > MAX_BUFFER_SIZE) {
            std::cout << "Buffer size exceeded maximum limit" << std::endl;
            sendErrorResponse(413);
            _requestBuffer.clear();
            return 1;
        }

        std::cout << BLUE << _webserv->getTimeStamp() 
                  << "Received " << bytesRead << " bytes from " << _fd 
                  << ", Total buffer: " << _requestBuffer.length() << " bytes" << RESET << "\n";

        if (_requestBuffer.find("\r\n\r\n") != std::string::npos || 
        _requestBuffer.find("\n\n") != std::string::npos) {

            Request req(_requestBuffer);
        
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
                
                // If partial upload, wait for more data
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
        return 0;
    } 
    else if (bytesRead == 0) {
        std::cout << RED << _webserv->getTimeStamp() 
                  << "Client disconnected: " << _fd << RESET << "\n";
        return 1;
    }
    else { // Error occurred
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

    if (req.getMethod() == "BAD") {
        sendErrorResponse(400);
    }

    return req;
}

int Client::processRequest(char *buffer) {
    Request req = parseRequest(buffer);
    if (req.getMethod() == "BAD") {
        return 1;
    }

    // Debug print
    std::cout << BLUE <<  _webserv->getTimeStamp() << "\n";
    std::cout << "Parsed Request:" << "\n";
    std::cout << "Method: " << req.getMethod() << "\n";
    std::cout << "Path: " << req.getPath() << "\n";
    std::cout << "Version: " << req.getVersion() << RESET << "\n\n";

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

    std::string scriptPath = req.getPath();
    std::string fullPath = _server->getWebRoot() + scriptPath;
    
    if (scriptPath.find("../") != std::string::npos) {
        sendErrorResponse(403);
        return 1;
    }

    if (_cgi.isCGIScript(scriptPath)) {
        return _cgi.executeCGI(*this, req, fullPath);
    }

    std::cout << _webserv->getTimeStamp() << "Handling GET request for path: " << req.getPath() << std::endl;

    std::string requestPath = req.getPath();
    if (requestPath == "/" || requestPath.empty()) {
        requestPath = "/index.html";
    }

    size_t end = requestPath.find_last_not_of(" \t\r\n");

    if (end != std::string::npos) {
        requestPath = requestPath.substr(0, end + 1);
    }

    // Construct full file path
    fullPath = _server->getWebRoot() + requestPath;

    std::cout << _webserv->getTimeStamp() << "Full file path: " << fullPath << "\n";

    // Check if file exists and is regular file
    struct stat fileStat;
    if (stat(fullPath.c_str(), &fileStat) != 0) {
        std::cout << _webserv->getTimeStamp() << "File not found: " << fullPath << "\n";
        sendErrorResponse(404);
        return 1;
    }


    if (!S_ISREG(fileStat.st_mode)) {
        std::cout << _webserv->getTimeStamp() << "Not a regular file: " << fullPath << std::endl;
        sendErrorResponse(403);
        return 1;
    }

    // Open file with additional error checking
    int fd = open(fullPath.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cout << _webserv->getTimeStamp() << "Failed to open file: " << fullPath << std::endl;
        sendErrorResponse(500);
        return 1;
    }

    // Clear any previous body content
    req.setBody("");

    // Read file contents
    char buffer[4096];
    ssize_t bytesRead;
    std::string fileContent;
    
    while ((bytesRead = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        fileContent += buffer;
    }
    close(fd);
    
    // Set the body content
    req.setBody(fileContent + "\r\n");

    // Check for read errors
    if (bytesRead < 0) {
        std::cout << "Error reading file: " << fullPath << std::endl;
        sendErrorResponse(500);
        return 1;
    }

    sendResponse(req, "keep-alive", req.getBody());

    std::cout << GREEN << _webserv->getTimeStamp() << "\nSuccessfully sent file: " << fullPath << std::endl;
    return 0;
}

std::string Client::extractFileName(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    
    if (pos == std::string::npos) {
        return path;
    }
    
    return path.substr(pos + 1);
}

int Client::handlePostRequest(Request& req) {
    std::string fullPath = _server->getWebRoot() + req.getPath();

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

    std::string uploadPath = _server->getUploadDir() + extractFileName(req.getPath());
    
    int fd = open(uploadPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        std::cout << _webserv->getTimeStamp() << "Failed to open file for writing: " << uploadPath << std::endl;
        sendErrorResponse(500);
        return 1;
    }

    std::cout << _webserv->getTimeStamp() << "Writing to file: " << uploadPath << "\n";
    std::cout << _webserv->getTimeStamp() << "Content to write: " << req.getQuery() << "\n";

    ssize_t bytesWritten = write(fd, req.getQuery().c_str(), req.getQuery().length());
    close(fd);

    if (bytesWritten < 0) {
        std::cout << "Failed to write to file" << "\n";
        sendErrorResponse(500);
        return 1;
    }

    sendResponse(req, "keep-alive", "");

    std::cout << _webserv->getTimeStamp() << "Successfully uploaded file: " << uploadPath << std::endl;
    return 0;
}

int Client::handleDeleteRequest(Request& req) {
    std::string fullPath = _server->getWebRoot() + req.getPath();

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
        return -1; // Need more data
    }
    
    if (!ensureUploadDirectory()) {
        sendErrorResponse(500);
        return 1;
    }
    
    if (!saveFile(filename, fileContent)) {
        sendErrorResponse(500);
        return 1;
    }
    
    std::string successMsg = "File uploaded successfully: " + filename;
    sendResponse(req, "keep-alive", successMsg);
    
    return 0;
}

bool Client::ensureUploadDirectory() {
    struct stat st;
    if (stat(_server->getUploadDir().c_str(), &st) != 0) {
        if (mkdir(_server->getUploadDir().c_str(), 0755) != 0) {
            std::cout << "Error: Failed to create upload directory" << std::endl;
            return false;
        }
    }
    return true;
}

bool Client::saveFile(const std::string& filename, const std::string& content) {
    std::string uploadPath = _server->getUploadDir() + filename;
    
    int fd = open(uploadPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        std::cout << "Error: Failed to open file for writing: " << uploadPath << std::endl;
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
        
        // Store the boundary
        req.setBoundary(boundary);
        
        // Verify the boundary was set correctly
        //std::cout << "Stored boundary: [" << req.getBoundary() << "]" << std::endl;
    }
}

ssize_t Client::sendResponse(Request req, std::string connect, std::string body) {
    std::string response = "HTTP/1.1 200 OK\r\n";
    
    // Get proper content type based on file extension
    std::string contentType = req.getContentType();
    if (contentType.find("multipart") == std::string::npos) {
        contentType = req.getMimeType(req.getPath());
    }
    
    response += "Content-Type: " + contentType + "\r\n";
    if (req.getMethod() == "POST") {
        response += "Content-Length: " + std::string(tostring(req.getQuery().length())) + "\r\n";
    } else {
        response += "Content-Length: " + std::string(tostring(req.getBody().length())) + "\r\n";
    }
    response += "Server: WebServ/1.0\r\n";
    response += "Connection: " + connect;
    response += "Access-Control-Allow-Origin: *";
    response += "\r\n\r\n";
    if (!body.empty()) {
        response += req.getBody();
        response += "\r\n\r\n";
    }
    
    return send(_fd, response.c_str(), response.length(), 0);
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
	Webserv &webserv = getWebserv();
	resolveErrorResponse(statusCode, webserv, statusText, body);
	std::string response = "HTTP/1.1 " + tostring(statusCode) + " " + statusText + "\r\n";
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: " + tostring(body.size()) + "\r\n";
	response += "Server: WebServ/1.0\r\n";
	response += "Connection: close\r\n";
	response += "\r\n";
	response += body;
	if (!send_all(_fd, response))
		std::cerr << "Failed to send error response" << std::endl;
}

void    Client::freeTokens(char **tokens) {
    for (int i = 0; tokens[i]; i++) {
        free(tokens[i]);
    }
    free(tokens);
}
