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
    if (_fd >= 0) {
        close(_fd);
        _fd = -1;
    }
    _requestBuffer.clear();
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
    char buffer[1024 * 1024];
    memset(buffer, 0, sizeof(buffer));
    
    int bytesRead = recv(_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead > 0) {
        _requestBuffer.append(buffer, bytesRead);

        if (_requestBuffer.length() > MAX_BUFFER_SIZE) {
            std::cout << "Buffer size exceeded maximum limit" << std::endl;
            sendErrorResponse(413, "File Too Large");
            _requestBuffer.clear();
            return 1;
        }

        std::cout << BLUE << _webserv->getTimeStamp() 
                  << "Received " << bytesRead << " bytes from " << _fd 
                  << ", Total buffer: " << _requestBuffer.length() << " bytes" << RESET << "\n";

        Request req(_requestBuffer);
        
        if (req.getMethod() == "BAD") {
            sendErrorResponse(400, "Bad Request");
            _requestBuffer.clear();
            return 1;
        }

        if (req.getContentType().find("multipart/form-data") != std::string::npos) {
            int result = handleMultipartPost(req);
            
            if (result != -1) {
                _requestBuffer.clear();
                return result;
            }
            return 0;
        }

        int processResult = processRequest(const_cast<char*>(_requestBuffer.c_str()));
        
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
        sendErrorResponse(400, "Bad Request");
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
        sendErrorResponse(405, "Method Not Allowed");
        return 1;
    }
}

int Client::handleGetRequest(Request& req) {

    std::string scriptPath = req.getPath();
    std::string fullPath = _server->getWebRoot() + scriptPath;
    
    if (scriptPath.find("../") != std::string::npos) {
        sendErrorResponse(403, "Forbidden");
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
        sendErrorResponse(404, "Not Found");
        return 1;
    }


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
    std::cout << _webserv->getTimeStamp() << "Content to write: " << req.getQuery() << "\n";

    ssize_t bytesWritten = write(fd, req.getQuery().c_str(), req.getQuery().length());
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
    std::string boundary = req.getBoundary();
    
    if (boundary.empty()) {
        sendErrorResponse(400, "Bad Request - No Boundary");
        return 1;
    }
    
    Multipart parser(_requestBuffer, boundary);
    
    if (!parser.parse()) {
        sendErrorResponse(400, "Bad Request - Invalid Format");
        return 1;
    }
    
    std::string filename = parser.getFilename();
    if (filename.empty()) {
        sendErrorResponse(400, "Bad Request - No Filename");
        return 1;
    }
    
    std::string fileContent = parser.getFileContent();
    if (fileContent.empty() && !parser.isComplete()) {
        return -1; // Need more data
    }
    
    if (!ensureUploadDirectory()) {
        sendErrorResponse(500, "Internal Server Error");
        return 1;
    }
    
    if (!saveFile(filename, fileContent)) {
        sendErrorResponse(500, "Internal Server Error");
        return 1;
    }
    
    std::string successMsg = "File uploaded successfully: " + filename;
    sendResponse(req, "keep-alive", successMsg);
    
    return 0;
}

ssize_t Client::sendResponse(Request req, std::string connect, std::string body) {
    std::string response = "HTTP/1.1 200 OK\r\n";
    
    // Get the correct content type based on the file extension
    std::string contentType = req.getMimeType(req.getPath());
    
    response += "Content-Type: " + contentType + "\r\n";
    
    // Handle content length
    size_t contentLength;
    if (!body.empty()) {
        contentLength = body.length();
    } else {
        contentLength = req.getBody().length();
    }
    
    response += "Content-Length: " + tostring(contentLength) + "\r\n";
    response += "Server: WebServ/1.0\r\n";
    response += "Connection: " + connect + "\r\n";
    response += "\r\n";
    
    if (!body.empty()) {
        response += body;
    } else {
        response += req.getBody();
    }
    
    // Debug output
    std::cout << "Sending response with Content-Type: " << contentType << std::endl;
    
    return send(_fd, response.c_str(), response.length(), 0);
}

void Client::sendErrorResponse(int statusCode, const std::string& message) {
    std::string response = "HTTP/1.1 " + tostring(statusCode) + " " + message + "\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "Content-Length: " + tostring(message.length()) + "\r\n";
    response += "Server: WebServ/1.0\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += message;
    
    send(_fd, response.c_str(), response.length(), 0);
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


void    Client::freeTokens(char **tokens) {
    for (int i = 0; tokens[i]; i++) {
        free(tokens[i]);
    }
    free(tokens);
}
