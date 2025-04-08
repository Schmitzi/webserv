#include "../include/Client.hpp"
#include "../include/Server.hpp"
#include "../include/Webserv.hpp"

Client::Client() : _webserv(NULL), _server(NULL) {

}

Client::Client(Webserv &other) {
    setWebserv(&other);
    setServer(&other.getServer());
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

int Client::acceptConnection() {
    _addrLen = sizeof(_addr);
    _fd = accept(_server->getFd(), (struct sockaddr *)&_addr, &_addrLen);
    if (_fd < 0) {
        _webserv->ft_error("Accept failed");
        return 1; // Try again
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
    send(_fd, "Connection established!\nEnter request: ", 40, 0);
}

int Client::recieveData() {
    memset(_buffer, 0, sizeof(_buffer));
    
    // Receive data
    int bytesRead = recv(_fd, _buffer, sizeof(_buffer) - 1, 0);
    
    if (bytesRead > 0) {
        std::cout << GREEN << "Recieved from " << _fd << ": " << _buffer << RESET << "\n";
        if (processRequest(_buffer) == 1) {
            return (0);
        }
        return (0);
    } 
    else if (bytesRead == 0) {
        std::cout << RED << "Client disconnected: " << _fd << RESET << "\n";
        return (1);
    }
    else { // Error occurred
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return (0);
        } else {
            return (_webserv->ft_error("recv() failed"), 1);
        }
    }
}

Request Client::parseRequest(char* buffer) {
    Request req;
    
    std::string input(buffer);
    input.erase(0, input.find_first_not_of(" \t\r\n"));
    input.erase(input.find_last_not_of(" \t\r\n") + 1);
    
    if (input.empty()) {
        req.setMethod("GET");
        req.setPath("/");
        req.setVersion("HTTP/1.1");
        return (req);
    }
    
    char** tokens = ft_split(buffer, ' ');
    
    if (tokens == NULL) {
        sendErrorResponse(400, "Bad Request");
        return (req);
    }
    
    if (std::string(tokens[0]) == "POST") {
        req.formatPost(_buffer);
    } else if (std::string(tokens[0]) == "DELETE") {
        req.formatDelete(tokens[1]);
    } else if (std::string(tokens[0]) == "GET") {
        req.formatGet(std::string(tokens[1]));
    } else {
        send(_fd, "Bad request\n", 13, 0);
        return req;
    }

    if (req.getPath().empty() || req.getPath()[0] != '/') {
        req.setPath("/" + req.getPath());
    }
    
    for (int i = 0; tokens[i]; i++) {
        free(tokens[i]);
    }
    free(tokens);
    
    return req;
}

int Client::processRequest(char *buffer) {
    Request req = parseRequest(buffer);

    // Debug print
    std::cout << "Parsed Request:" << std::endl;
    std::cout << "Method: " << req.getMethod() << std::endl;
    std::cout << "Path: " << req.getPath() << std::endl;
    std::cout << "Version: " << req.getVersion() << "\n" << std::endl;

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
    std::cout << "Handling GET request for path: " << req.getPath() << std::endl;

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
    std::string fullPath = _server->getWebRoot() + requestPath;

    // Debug print full path
    std::cout << "Full file path: " << fullPath << std::endl;

    // Check if file exists and is regular file
    struct stat fileStat;
    if (stat(fullPath.c_str(), &fileStat) != 0) {
        std::cout << "File not found: " << fullPath << std::endl;
        sendErrorResponse(404, "Not Found");
        return 1;
    }

    // Additional security checks
    if (!S_ISREG(fileStat.st_mode)) {
        std::cout << "Not a regular file: " << fullPath << std::endl;
        sendErrorResponse(403, "Forbidden");
        return 1;
    }

    // Open file with additional error checking
    int fd = open(fullPath.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cout << "Failed to open file: " << fullPath << std::endl;
        sendErrorResponse(500, "Internal Server Error");
        return 1;
    }

    // Determine mime type based on file extension
    std::string contentType = "text/";
    size_t dotPos = requestPath.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string ext = requestPath.substr(dotPos);
        if (ext != ".html")
            contentType += ext;
    }

    // Read file contents
    char buffer[4096];
    ssize_t bytesRead;
    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
        req.setBody(req.getBody() + buffer);
    }
    close(fd);

    // Check for read errors
    if (bytesRead < 0) {
        std::cout << "Error reading file: " << fullPath << std::endl;
        sendErrorResponse(500, "Internal Server Error");
        return 1;
    }

    // Prepare and send HTTP response
    std::string response = "\nHTTP/1.1 200 OK\r\n";
    response += "Content-Length: " + ft_itoa(req.getBody().length()) + "\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Server: WebServ/1.0\r\n";
    response += "Connection: close\r\n\r\n";
    response += req.getBody();
    response += "\n";

    // Send response
    ssize_t bytesSent = send(_fd, response.c_str(), response.length(), 0);
    if (bytesSent < 0) {
        std::cout << "Failed to send response" << std::endl;
        return 1;
    }

    std::cout << "\nSuccessfully sent file: " << fullPath << std::endl;
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

    // Sanitize path to prevent directory traversal
    if (req.getPath().find("../") != std::string::npos) {
        sendErrorResponse(403, "Forbidden");
        return 1;
    }

    std::string uploadPath = _server->getUploadDir() + extractFileName(req.getPath());
    
    int fd = open(uploadPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        std::cout << "Failed to open file for writing: " << uploadPath << std::endl;
        sendErrorResponse(500, "Internal Server Error");
        return 1;
    }

    std::cout << "Writing to file: " << uploadPath << "\n";
    std::cout << "Content to write: " << req.getBody() << "\n";

    ssize_t bytesWritten = write(fd, req.getBody().c_str(), req.getBody().length());
    close(fd);

    if (bytesWritten < 0) {
        std::cout << "Failed to write to file" << "\n";
        sendErrorResponse(500, "Internal Server Error");
        return 1;
    }

    sendResponse(req);

    std::cout << "Successfully uploaded file: " << uploadPath << std::endl;
    return 0;
}

int Client::handleDeleteRequest(Request& req) {
    std::string fullPath = _server->getWebRoot() + req.getPath();

    size_t end = fullPath.find_last_not_of(" \t\r\n");
    if (end != std::string::npos) {
        fullPath = fullPath.substr(0, end + 1);
    }

    if (unlink(fullPath.c_str()) != 0) {
        sendErrorResponse(403, "Forbidden");
        return 1;
    }

    // Send success response
    std::string response = "HTTP/1.1 200 OK\r\n\r\n";
    send(_fd, response.c_str(), response.length(), 0);
    return 0;
}

void    Client::sendResponse(Request req) {
    std::string response = "HTTP/1.1 201 OK\r\n";
    response += "Content-Length: " + std::string(ft_itoa(req.getBody().length()));
    response +=  "\r\n";
    response += "Connection: close\r\n\r\n";
    send(_fd, response.c_str(), response.length(), 0);
}

void Client::sendErrorResponse(int statusCode, const std::string& message) {
    std::string response = "HTTP/1.1 " + ft_itoa(statusCode) + " " + message + "\r\n\r\n";
    send(_fd, response.c_str(), response.length(), 0);
}
