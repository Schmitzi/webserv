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
    char buffer[1024 * 1024];  // SEt buffer to 1MB
    //memset(buffer, 0, sizeof(buffer));
    
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

        std::cout << BLUE << _requestBuffer << RESET << "\n";
        // Handle multipart uploads separately
        if (_requestBuffer.find("multipart/form-data") != std::string::npos) {
            Request req(_requestBuffer);

            std::cout << RED << "TEst OUtput\n" << RESET;
            std::cout << BLUE << "Method: "  << req.getMethod() << "\n";
            std::cout << "Path: "  << req.getPath() << "\n";
            std::cout << "Content Type: "  << req.getContentType() << "\n";
            std::cout << "Boundary: "  << req.getBoundary() << "\n";
            std::cout << "Version: "  << req.getVersion() << "\n";
            std::cout << "Query: "  << req.getQuery() << "\n";
            std::cout << "Body: "  << req.getBody() << "\n" << RESET;

            // std::map<std::string, std::string> temp = mapSplit(split(_requestBuffer, '\n'));
            // req.setHeader(temp);
            
            // if (req.getHeaders().find("boundary=") != std::string::npos) {

            // }

            // size_t boundaryPos = _requestBuffer.find("boundary=");
            // if (boundaryPos != std::string::npos) {
            //     size_t boundaryEnd = _requestBuffer.find("\r\n", boundaryPos);
            //     if (boundaryEnd == std::string::npos) {
            //         boundaryEnd = _requestBuffer.length();
            //     }
            //     std::string boundary = _requestBuffer.substr(
            //         boundaryPos + 9, 
            //         boundaryEnd - (boundaryPos + 9)
            //     );
            //     // Remove quotes if present
            //     if (boundary[0] == '"') {
            //         boundary = boundary.substr(1, boundary.length() - 2);
            //     }
            //     req.setBoundary(boundary);
            //     req.setContentType("multipart/form-data; boundary=" + boundary);
            // }
            
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
    Request req(buffer);

    // Safely convert buffer to std::string
    std::string input;
    if (buffer) {
        size_t len = strlen(buffer);
        input.assign(buffer, len);
    }

    // Trim trailing whitespace and newlines
    while (!input.empty()) {
        char lastChar = input[input.length() - 1];
        if (lastChar == '\n' || lastChar == '\r' || 
            lastChar == ' ' || lastChar == '\t') {
            input.resize(input.length() - 1);
        } else {
            break;
        }
    }

    findContentType(req);
    
    if (input.empty()) {
        return req;
    }

    std::vector<std::string> raw = split(input, '\n');
    
    // Ensure we have at least one line
    if (raw.empty()) {
        sendErrorResponse(400, "Bad Request");
        return req;
    }

    std::vector<std::string> tokens = split(raw[0], ' ');
    req.setHeader(mapSplit(raw));
    
    if (tokens.size() < 2) {
        sendErrorResponse(400, "Bad Request");
        return req;
    }

    std::string path = "/";

    if (!tokens[1].empty()) {
        path = tokens[1];
        // Remove trailing whitespace
        size_t end = path.find_last_not_of(" \t\r\n");
        if (end != std::string::npos) {
            path = path.substr(0, end + 1);
        }
    }
    
    try {
        if (tokens[0] == "POST") {
            req.formatPost(tokens[1]);
        } else if (tokens[0] == "DELETE") {
            if (!tokens[1].empty()) {
                req.formatDelete(path);
            } else {
                sendErrorResponse(400, "Bad Request - Missing path");
            }
        } else if (tokens[0] == "GET" || tokens[0] == "curl") {
            if (!tokens[1].empty()) {
                if (req.formatGet(path) == 1) {
                    sendErrorResponse(403, "Bad request\n");
                    return req;
                }
            } else {
                req.formatGet("/index.html");
            }
        } else {
            sendErrorResponse(403, "Bad request\n");
            std::cout << RED << _webserv->getTimeStamp() << "Client " << _fd 
                      << ": 403 Bad request\n" << RESET;
            req.setMethod("BAD");
            return req;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception in parseRequest: " << e.what() << std::endl;
        sendErrorResponse(500, "Internal Server Error");
        req.setMethod("BAD");
        return req;
    }

    // Ensure path starts with '/'
    if (req.getPath().empty() || req.getPath()[0] != '/') {
        req.setPath("/" + req.getPath());
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
    std::string& raw = _requestBuffer;
    std::string boundary = req.getBoundary();    
    
    if (boundary.empty()) {
        std::cout << "Error: No boundary found for multipart/form-data request" << std::endl;
        sendErrorResponse(400, "Bad Request - No Boundary");
        return 1;
    }
    
    // The boundary in the body will have -- prefix
    std::string fullBoundary = "--" + boundary;
    
    // Extract filename
    std::vector<std::string> lines = split(raw, '\n');
    std::map<std::string, std::string> headers = mapSplit(lines);
    
    // std::string contentDisposition = headers["Content-Disposition"];
    std::string filename;
    
    size_t filenamePos = req.getContentDisp().find("filename=\"");
    if (filenamePos != std::string::npos) {
        filenamePos += 10; // Skip 'filename="'
        size_t filenameEnd = req.getContentDisp().find("\"", filenamePos);
        if (filenameEnd != std::string::npos) {
            req.setFileName(req.getContentDisp().substr(filenamePos, filenameEnd - filenamePos));
        }
    }
    
    if (filename.empty()) {
        std::cout << "Error: Filename not found" << std::endl;
        sendErrorResponse(400, "Bad Request - No Filename");
        return 1;
    }
    
    // Find the start of the file content
    size_t contentStart = raw.find("\r\n\r\n");
    if (contentStart == std::string::npos) {
        contentStart = raw.find("\n\n");
        if (contentStart == std::string::npos) {
            std::cout << "Error: Could not find start of file content" << std::endl;
            sendErrorResponse(400, "Bad Request - Invalid Format");
            return 1;
        }
        contentStart += 2;
    } else {
        contentStart += 4;
    }

    std::cout << RED << "TEST\n" << RESET;
    
    // Create upload directory if it doesn't exist
    struct stat st;
    if (stat(_server->getUploadDir().c_str(), &st) != 0) {
        if (mkdir(_server->getUploadDir().c_str(), 0755) != 0) {
            std::cout << "Error: Failed to create upload directory" << std::endl;
            sendErrorResponse(500, "Internal Server Error");
            return 1;
        }
    }
    
    // Save the file using a 1MB buffer
    std::string uploadPath = _server->getUploadDir() + filename;
    int fd = open(uploadPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        std::cout << "Error: Failed to open file for writing: " << uploadPath << std::endl;
        std::cout << "Error details: " << strerror(errno) << std::endl;
        sendErrorResponse(500, "Internal Server Error");
        return 1;
    }
    
    // Write the first chunk from the current buffer
    std::string initialContent = raw.substr(contentStart);
    
    // Find the end boundary
    size_t contentEnd = initialContent.find(fullBoundary + "--");
    
    if (contentEnd == std::string::npos) {
        // Partial upload - write current content and prepare for more
        ssize_t bytesWritten = write(fd, initialContent.c_str(), initialContent.length());
        
        if (bytesWritten < 0) {
            close(fd);
            std::cout << "Error writing file: " << strerror(errno) << std::endl;
            sendErrorResponse(500, "Internal Server Error");
            return 1;
        }
        
        close(fd);
        return -1; // Indicates more data is needed
    }
    
    // Write the file content before the end boundary
    std::string fileContent = initialContent.substr(0, contentEnd);
    
    // Remove trailing whitespace, newlines, and the last boundary
    while (!fileContent.empty()) {
        char lastChar = fileContent[fileContent.length() - 1];
        if (lastChar == '\r' || lastChar == '\n' || lastChar == ' ' || lastChar == '\t') {
            fileContent.resize(fileContent.length() - 1);
        } else {
            break;
        }
    }
    
    // Write the file content
    ssize_t bytesWritten = write(fd, fileContent.c_str(), fileContent.length());
    close(fd);
    
    if (bytesWritten < 0) {
        std::cout << "Error: Failed to write to file: " << strerror(errno) << std::endl;
        sendErrorResponse(500, "Internal Server Error");
        return 1;
    }
    
    // Send success response
    std::string successMsg = "File uploaded successfully: " + filename;
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "Content-Length: " + ft_itoa(successMsg.length()) + "\r\n";
    response += "Server: WebServ/1.0\r\n";
    response += "Connection: keep-alive\r\n";
    response += "\r\n";
    response += successMsg;
    
    send(_fd, response.c_str(), response.length(), 0);
    
    // Clear the request buffer after successful upload
    _requestBuffer.clear();
    
    return 0;
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
    
    
    // Check for boundary parameter
    size_t boundaryPos = contentType.find("; boundary=");
    if (boundaryPos != std::string::npos) {
        std::string boundary = contentType.substr(boundaryPos + 11); // +11 for "; boundary="
        
        // Clean up boundary if needed
        size_t quotePos = boundary.find("\"");
        if (quotePos != std::string::npos) {
            boundary = boundary.substr(0, quotePos);
        }
        
        
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
