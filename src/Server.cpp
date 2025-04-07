#include "../include/Server.hpp"
#include "../include/Webserv.hpp"

Server::Server() : _webserv(NULL) {

}

Server::~Server() {

}

void Server::setWebserv(Webserv* webserv) {
    _webserv = webserv;
}

int Server::openSocket() { // Create a TCP socket
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd < 0) {
        _webserv->ft_error("Socket creation error");
        return 1;
    }
    _webserv->printMsg("Server started", GREEN, "");
    return 0;
}

int Server::setOptional() { // Optional: set socket options to reuse address
    int opt = 1;
    if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        _webserv->ft_error("setsockopt() failed");
        close(_fd);
        return 1;
    }
    return 0;
}

int Server::setServerAddr() { // Set up server address
    memset(&_addr, 0, sizeof(_addr));
    _addr.sin_family = AF_INET;          // IPv4 Internet Protocol
    _addr.sin_addr.s_addr = INADDR_ANY;  // Accept connections on any interface
    _addr.sin_port = htons(8080);        // Port 8080 (pull port from config)
    return 0;
}

int Server::ft_bind() { // Bind socket to address
    if (bind(_fd, (struct sockaddr *)&_addr, sizeof(_addr)) < 0) {
        _webserv->ft_error("bind() failed");
        close(_fd);
        return 1;
    }
    return 0;
}

int Server::ft_listen() { // Listen for connections
    if (listen(_fd, 3) < 0) {
        _webserv->ft_error("listen() failed");
        close(_fd);
        return 1;
    }
    return 0;
}