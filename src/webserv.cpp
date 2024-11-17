#include "../include/webserv.hpp"

// Othodox Cannonical Form

Webserv::Webserv() {
    std::cout << "Webserv constructed" << std::endl;
}

Webserv::Webserv(const std::string& config) {
   std::cout << "Webserv copy constructed" << std::endl;
   _config = config;
}

Webserv& Webserv::operator=(Webserv W) {
    std::swap(*this, W);
	return *this;
}

Webserv::~Webserv() {
    std::cout << "Webserv deconstructed" << std::endl;
}

void    Webserv::setup() {
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr = INADDR_ANY;
    _address.sin_port = htons(8080);

    if (bind(_server_fd, (struct sockaddr*)&_address, sizeof(_address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(_server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
}

int Webserv::run() {
    std::cout << "Server is running on port 8080..." << std::endl;
    while (true) {
        int new_socket = accept(_server_fd, NULL, NULL);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
        send(new_socket, response, strlen(response), 0);
        close(new_socket);
    }
}
