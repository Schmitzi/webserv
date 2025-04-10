#include "../include/Client.hpp"
#include "../include/Server.hpp"
#include "../include/Webserv.hpp"

Client::Client() : _webserv(NULL), _server(NULL) {

}

Client::Client(Webserv &other) {
    setWebserv(&other);
    setServer(&other.getServer());

    _cgi.setPythonPath(other.getEnvironment());
    _cgi.setClient(*this);
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

std::string const &Client::getRawData() {
    return _rawData;
}


void Client::setWebserv(Webserv *webserv) {
    _webserv = webserv;
}

void Client::setServer(Server *server) {
    _server = server;
}

void    Client::setRawData(std::string const data) {
    _rawData = data;
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
    memset(_buffer, 0, sizeof(_buffer));
    
    // Receive data
    int bytesRead = recv(_fd, _buffer, sizeof(_buffer) - 1, 0);
    _rawData.append(_buffer, bytesRead);
    
    if (bytesRead > 0) {
        std::cout << GREEN << _webserv->getTimeStamp() << "Received data from " 
                 << _fd << " (" << bytesRead << " bytes)" << RESET << "\n";
        
        if (isRequestComplete(_rawData)) {
            std::cout << "Complete request data: " << std::endl << _rawData << std::endl;
            processRequest(_rawData.c_str());
            _rawData.clear();
        }
        return 0;
    } 
    else if (bytesRead == 0) {
        std::cout << RED << _webserv->getTimeStamp() << "Client disconnected: " 
                 << _fd << RESET << "\n";
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

Request Client::parseRequest(char const *buffer) {
    Request req;
    std::string temp(buffer);
    
    // Find headers end
    size_t headersEnd = temp.find("\r\n\r\n");
    if (headersEnd == std::string::npos) {
        headersEnd = temp.find("\n\n");
    }

    if (headersEnd != std::string::npos) {
        // Parse headers
        std::istringstream headerStream(temp.substr(0, headersEnd));
        std::string line;
        std::getline(headerStream, line); // First line with method, path, etc.

        char** tokens = ft_split(line.c_str(), ' ');
        
        if (tokens == NULL) {
            sendErrorResponse(400, "Bad Request");
            return req;
        }

        std::string path = "/";
        if (tokens[1]) {
            path = tokens[1];
            path.erase(path.find_last_not_of(" \t\r\n") + 1);
        }
        
        // Parse other headers
        std::string contentType;
        std::string contentLength;
        while (std::getline(headerStream, line) && !line.empty()) {
            size_t colonPos = line.find(": ");
            if (colonPos != std::string::npos) {
                std::string headerName = line.substr(0, colonPos);
                std::string headerValue = line.substr(colonPos + 2);
                
                if (headerName == "Content-Type") {
                    req.setContentType(headerValue);
                    contentType = headerValue;
                }
                if (headerName == "Content-Length") {
                    contentLength = headerValue;
                }
            }
        }
        
        // Set body for multipart/form-data
        if (contentType.find("multipart/form-data") != std::string::npos) {
            req.setBody(temp.substr(headersEnd + 4)); // include body after headers
        }
        
        if (std::string(tokens[0]) == "POST") {
            req.formatPost(tokens[1]);
        } else if (std::string(tokens[0]) == "DELETE") {
            if (tokens[1]) {
                req.formatDelete(path);
            } else {
                sendErrorResponse(400, "Bad Request - Missing path");
            }
        } else if (std::string(tokens[0]) == "GET" || std::string(tokens[0]) == "curl") {
            if (tokens[1]) {
                if (req.formatGet(path) == 1) {
                    sendErrorResponse(403, "Bad request\n");
                    freeTokens(tokens);
                    return req;
                }
            } else {
                req.formatGet("/index.html");
            }
        } else {
            sendErrorResponse(403, "Bad request\n");
            freeTokens(tokens);
            std::cout << RED << _webserv->getTimeStamp() << "Client " << _fd << ": 403 Bad request\n" << RESET;
            req.setMethod("BAD");
            return req;
        }

        if (req.getPath().empty() || req.getPath()[0] != '/') {
            req.setPath("/" + req.getPath());
        }
        
        freeTokens(tokens);
    }
    
    return req;
}

int Client::processRequest(char const *buffer) {
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
        sendErrorResponse(405, "Method Not Allowed");
        return 1;
    }
}

int Client::handleGetRequest(Request& req) {

    // First, check if the requested path is a CGI script
    std::string scriptPath = req.getPath();
    std::string fullPath = _server->getWebRoot() + scriptPath;
    
    if (_cgi.isCGIScript(scriptPath)) {
        // If it's a CGI script, execute it
        return _cgi.executeCGI(*this, req, fullPath);
    }

    // If not a CGI script, continue with normal file serving (your existing code)
    std::cout << _webserv->getTimeStamp() << "Handling GET request for path: " << req.getPath() << std::endl;

    // Sanitize path to prevent directory traversal
    if (req.getPath().find("../") != std::string::npos) {
        sendErrorResponse(403, "Forbidden");
        return 1;
    }

    // Handle default index if root path is requested
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

    // Debug print full path
    std::cout << _webserv->getTimeStamp() << "Full file path: " << fullPath << "\n";

    // Check if file exists and is regular file
    struct stat fileStat;
    if (stat(fullPath.c_str(), &fileStat) != 0) {
        std::cout << _webserv->getTimeStamp() << "File not found: " << fullPath << "\n";
        sendErrorResponse(404, "Not Found");
        return 1;
    }

    // Additional security checks
    if (!S_ISREG(fileStat.st_mode)) {
        std::cout << _webserv->getTimeStamp() << "Not a regular file: " << fullPath << std::endl;
        sendErrorResponse(403, "Forbidden");
        return 1;
    }

    // Open file with additional error checking
    int fd = open(fullPath.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cout << _webserv->getTimeStamp() << "Failed to open file: " << fullPath << std::endl;
        sendErrorResponse(500, "Internal Server Error");
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
        sendErrorResponse(500, "Internal Server Error");
        return 1;
    }

    sendResponse(req, "keep-alive", req.getBody());

    std::cout << _webserv->getTimeStamp() << "\nSuccessfully sent file: " << fullPath << std::endl;
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

    if (req.getContentType().find("multipart/form-data") != std::string::npos) {
        std::cout << RED << "\n\nHandling multipart/form-data upload\n\n" << RESET; 
        return handleMultipartPost(req);
    }

    if (req.getPath().find("../") != std::string::npos) {
        sendErrorResponse(403, "Forbidden");
        return 1;
    }

    std::string uploadPath = _server->getUploadDir() + extractFileName(req.getPath());
    
    int fd = open(uploadPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        std::cout << _webserv->getTimeStamp() << "Failed to open file for writing: " << uploadPath << std::endl;
        sendErrorResponse(500, "Internal Server Error");
        return 1;
    }

    std::cout << _webserv->getTimeStamp() << "Writing to file: " << uploadPath << "\n";
    std::cout << _webserv->getTimeStamp() << "Content to write: " << req.getQuery() << "\n"; // body?

    ssize_t bytesWritten = write(fd, req.getQuery().c_str(), req.getQuery().length()); // Maybe body?
    close(fd);

    if (bytesWritten < 0) {
        std::cout << "Failed to write to file" << "\n";
        sendErrorResponse(500, "Internal Server Error");
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
        sendErrorResponse(403, "Forbidden");
        return 1;
    }

    sendResponse(req, "keep-alive", "");
    return 0;
}

int Client::handleMultipartPost(Request& req) {
    std::string body = req.getBody();
    if (body.empty()) {
        std::cout << _webserv->getTimeStamp() << "Empty body in multipart request" << std::endl;
        sendErrorResponse(400, "Bad Request - Empty Body");
        return 1;
    }

    // Print full body content for debugging
    std::cout << "Full Body Content (length " << body.length() << "):\n---\n" 
              << body << "\n---\n";

    // Extract boundary from Content-Type
    std::string contentType = req.getContentType();
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        std::cout << _webserv->getTimeStamp() << "No boundary in multipart request" << std::endl;
        sendErrorResponse(400, "Bad Request - No Boundary");
        return 1;
    }

    // Get the boundary string
    std::string boundary = contentType.substr(boundaryPos + 9); // +9 for "boundary="
    boundary = "--" + boundary; // Boundaries in the body start with --

    // Debug print boundary
    std::cout << "Boundary: [" << boundary << "]" << std::endl;

    // Find filename in Content-Disposition header
    size_t filenamePos = body.find("filename=\"");
    if (filenamePos == std::string::npos) {
        std::cout << _webserv->getTimeStamp() << "No filename in multipart request" << std::endl;
        sendErrorResponse(400, "Bad Request - No Filename");
        return 1;
    }

    // Extract filename
    filenamePos += 10; // Skip 'filename="'
    size_t filenameEnd = body.find("\"", filenamePos);
    std::string filename = body.substr(filenamePos, filenameEnd - filenamePos);

    // Find start of file content (after the blank line following the Content-Type)
    size_t contentTypePos = body.find("Content-Type:", filenameEnd);
    if (contentTypePos == std::string::npos) {
        std::cout << _webserv->getTimeStamp() << "No Content-Type in multipart request" << std::endl;
        sendErrorResponse(400, "Bad Request - Invalid Format");
        return 1;
    }

    // Find content start (after headers) - try both CRLF and LF
    size_t contentStart = body.find("\r\n\r\n", contentTypePos);
    if (contentStart == std::string::npos) {
        contentStart = body.find("\n\n", contentTypePos);
        if (contentStart == std::string::npos) {
            std::cout << _webserv->getTimeStamp() << "Cannot find content start in multipart request" << std::endl;
            sendErrorResponse(400, "Bad Request - Invalid Format");
            return 1;
        }
        contentStart += 2; // Skip \n\n
    } else {
        contentStart += 4; // Skip \r\n\r\n
    }

    // Find end of file content 
    size_t boundaryStart = body.find(boundary + "--", contentStart);
    if (boundaryStart == std::string::npos) {
        std::cout << _webserv->getTimeStamp() << "Cannot find content end in multipart request" << std::endl;
        sendErrorResponse(400, "Bad Request - Invalid Format");
        return 1;
    }

    // Extract file content
    std::string fileContent = body.substr(contentStart, boundaryStart - contentStart);
    
    // Remove trailing whitespace/newlines
    size_t contentEnd = fileContent.find_last_not_of(" \r\n");
    if (contentEnd != std::string::npos) {
        fileContent = fileContent.substr(0, contentEnd + 1);
    }

    // Save the file
    std::string uploadPath = _server->getUploadDir() + filename;
    
    int fd = open(uploadPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        std::cout << _webserv->getTimeStamp() << "Failed to open file for writing: " << uploadPath << std::endl;
        sendErrorResponse(500, "Internal Server Error");
        return 1;
    }

    std::cout << _webserv->getTimeStamp() << "Writing to file: " << uploadPath << std::endl;
    std::cout << _webserv->getTimeStamp() << "Content to write: " << fileContent << std::endl;

    ssize_t bytesWritten = write(fd, fileContent.c_str(), fileContent.length());
    close(fd);

    if (bytesWritten < 0) {
        std::cout << "Failed to write to file" << std::endl;
        sendErrorResponse(500, "Internal Server Error");
        return 1;
    }

    // Create and send success response
    std::string successMsg = "File uploaded successfully: " + filename;
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "Content-Length: " + ft_itoa(successMsg.length()) + "\r\n";
    response += "Server: WebServ/1.0\r\n";
    response += "Connection: keep-alive\r\n";
    response += "\r\n";
    response += successMsg;
    
    send(_fd, response.c_str(), response.length(), 0);

    std::cout << _webserv->getTimeStamp() << "Successfully uploaded file: " << uploadPath << std::endl;
    return 0;
}

bool Client::isRequestComplete(std::string &data) {
    if (data.find("Content-Type: multipart/form-data") != std::string::npos) {
        size_t boundaryStart = data.find("boundary=");
        if (boundaryStart != std::string::npos) {
            size_t end = data.find("\r\n", boundaryStart);
            std::string boundary = data.substr(boundaryStart + 9, end - (boundaryStart + 9));
            std::string reqEnd = "--" + boundary + "--";
            return data.find(reqEnd) != std::string::npos;
        }
    }
    
    // For other requests, check if we've received the full header and body
    if (data.find("\r\n\r\n") != std::string::npos) {
        // For requests with Content-Length, check if we've received all the data
        size_t contentLengthPos = data.find("Content-Length: ");
        if (contentLengthPos != std::string::npos) {
            size_t valueStart = contentLengthPos + 16; // Length of "Content-Length: "
            size_t valueEnd = data.find("\r\n", valueStart);
            std::string lengthStr = data.substr(valueStart, valueEnd - valueStart);
            size_t contentLength = atoi(lengthStr.c_str());
            
            // Find the body start
            size_t bodyStart = data.find("\r\n\r\n") + 4;
            return (data.length() - bodyStart) >= contentLength;
        }
        return true; // No Content-Length, assume complete after headers
    }
    return false; // Haven't found end of headers yet
}

ssize_t Client::sendResponse(Request req, std::string connect, std::string body) {
    std::string response = "HTTP/1.1 200 OK\r\n";
    
    // Get proper content type based on file extension
    std::string contentType = req.getMimeType(req.getPath());
    
    response += "Content-Type: " + contentType + "\r\n";
    if (req.getMethod() == "POST") {
        response += "Content-Length: " + std::string(ft_itoa(req.getQuery().length())) + "\r\n";
    } else {
        response += "Content-Length: " + std::string(ft_itoa(req.getBody().length())) + "\r\n";
    }
    response += "Server: WebServ/1.0\r\n";
    response += "Connection: " + connect;
    response += "\r\n\r\n";
    if (!body.empty()) {
        response += req.getBody();
        response += "\r\n\r\n";
    }
    
    return send(_fd, response.c_str(), response.length(), 0);
}

void Client::sendErrorResponse(int statusCode, const std::string& message) {
    // Create a more complete HTTP response with all required headers
    std::string response = "HTTP/1.1 " + ft_itoa(statusCode) + " " + message + "\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "Content-Length: " + ft_itoa(message.length()) + "\r\n";
    response += "Server: WebServ/1.0\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += message;  // Add the message as the response body
    
    send(_fd, response.c_str(), response.length(), 0);
}

void    Client::freeTokens(char **tokens) {
    for (int i = 0; tokens[i]; i++) {
        free(tokens[i]);
    }
    free(tokens);
}
