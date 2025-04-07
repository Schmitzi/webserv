#include "../include/Webserv.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/Config.hpp"

// Othodox Cannonical Form

Webserv::Webserv() {
    //std::cout << "Webserv constructed" << std::endl;
}

Webserv::Webserv(std::string const &config) {
   std::cout << "Webserv copy constructed" << std::endl;
   _config = config;
}

Webserv::Webserv(Webserv const &other) {
    _config = other._config;
}

Webserv &Webserv::operator=(Webserv const &other) {
    _config = other._config;
	return *this;
}

Webserv::~Webserv() {
    //std::cout << "Webserv deconstructed" << std::endl;
}

int Webserv::setConfig(std::string const filepath) {
    std::cout << getTimeStamp() << "Config found at " << filepath << "\n";
    return true;
}

int Webserv::openSocket() { // Create a TCP socket
    _server._fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server._fd < 0) {
        ft_error("Socket creation error");
        return 1;
    }
    std::cout << getTimeStamp() << "Server started\n";
    return 0;
}

int Webserv::setOptional() { // Optional: set socket options to reuse address
    int opt = 1;
    if (setsockopt(_server._fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        ft_error("setsockopt() failed");
        close(_server._fd);
        return 1;
    }
    return 0;
}

int Webserv::setServerAddr() { // Set up server address
    memset(&_server._addr, 0, sizeof(_server._addr));
    _server._addr.sin_family = AF_INET;          // IPv4 Internet Protocol
    _server._addr.sin_addr.s_addr = INADDR_ANY;  // Accept connections on any interface
    _server._addr.sin_port = htons(8080);        // Port 8080
    return 0;
}

int Webserv::ft_bind() { // Bind socket to address
    if (bind(_server._fd, (struct sockaddr *)&_server._addr, sizeof(_server._addr)) < 0) {
        ft_error("bind() failed");
        close(_server._fd);
        return 1;
    }
    return 0;
}

int Webserv::ft_listen() { // Listen for connections
    if (listen(_server._fd, 3) < 0) {
        ft_error("listen() failed");
        close(_server._fd);
        return 1;
    }
    return 0;
}

int Webserv::run() {
    if (openSocket() || setOptional() || setServerAddr() || ft_bind() || ft_listen()) {
        return 1;
    }

    std::cout << getTimeStamp() << "Server is listening on port " << "8080" << "...\n"; //_address.sin_port

    while (1) { // Accept connections and handle them in a loop
        
        _client._addrLen = sizeof(_client._addr);
        _client._fd = accept(_server._fd, (struct sockaddr *)&_client._addr, &_client._addrLen);
        if (_client._fd < 0) {
            ft_error("Accept failed");
            continue; // Try again
        }

        // Display client connection info
        _client.ip = (unsigned char *)&_client._addr.sin_addr.s_addr;
        std::cout << getTimeStamp() << "New connection from " << _client.ip[0] << "." << _client.ip[1] \
            << "." << _client.ip[2] << "." << _client.ip[3] << ":" << ntohs(_client._addr.sin_port) << "\n";

        // Handle communication with this client
        char buffer[1024];
        while (1) {
            // Clear buffer
            memset(buffer, 0, sizeof(buffer));

            // Receive data
            int bytes_read = recv(_client._fd, buffer, sizeof(buffer) - 1, 0);
            if (bytes_read <= 0) {
                if (bytes_read == 0) {
                    std::cout << getTimeStamp() << "Client disconnected" << std::endl;
                } else {
                    ft_error("recv() failed");
                }
                break;
            }

            // Process received command
            std::cout << getTimeStamp() << "Received: " << buffer << "\n";

            // Echo the command back to client
            std::string response_str = "Server received: ";
            response_str += buffer;
            send(_client._fd, response_str.c_str(), response_str.length(), 0);
        }
        close(_client._fd);
    }
    close(_server._fd);
    return 0;
}

void    Webserv::ft_error(std::string const msg) {
    std::cerr << getTimeStamp() << "Error: " << msg << ": " << strerror(errno) << "\n";
}


std::string Webserv::getTimeStamp() {
    // Use poll with a timeout of 0 to get current time
    struct pollfd pfd;
    pfd.fd = -1;  // Invalid fd
    pfd.events = POLLIN;
    
    // Poll returns immediately due to invalid fd
    poll(&pfd, 1, 1);  // Wait for 1ms max    NEEDS TO BE CHANGED
    
    // Get current time manually
    time_t now = time(NULL);  // time() is a standard function
    struct tm* tm_info = localtime(&now);
    
    std::ostringstream oss;
    oss << "[" 
        << (tm_info->tm_year + 1900) << "-"
        << (tm_info->tm_mon + 1) << "-"
        << tm_info->tm_mday << " "
        << tm_info->tm_hour << ":"
        << tm_info->tm_min << ":"
        << tm_info->tm_sec << "] ";
    
    return oss.str();
}