#include "../include/Server.hpp"
#include "../include/Webserv.hpp"

Server::Server() : _uploadDir("local/upload/"), _webRoot("local"), _webserv(NULL) {

}

Server::~Server()  {

}

Webserv &Server::getWebServ() {
    return *_webserv;
}

struct sockaddr_in  &Server::getAddr() {
    return _addr;
}

int &Server::getFd() {
    return _fd;
}

// std::vector<int>& Server::getFds() {
// 	return _fds;
// }

std::string const &Server::getUploadDir() {
    return _uploadDir;
}

std::string const   &Server::getWebRoot() {
    return _webRoot;
}

// void Server::addListenPort(const std::string& ip, int port) {
// 	_listenPorts.push_back(std::pair<std::string, int>(ip, port));
// }

void Server::setWebserv(Webserv* webserv) {
    _webserv = webserv;
}

void    Server::setFd(int const fd) {
    _fd = fd;
}

int Server::openSocket() { // Create a TCP socket
    _fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);//TODO: added SOCK_NONBLOCK because we shouldnt use fcntl
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
	_addr.sin_port = htons(getWebServ().getConfig().getPort());
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

//TODO: just a try
// int Server::openAndListenSockets() {
// 	std::vector<std::pair<std::string, int> >::iterator it = _listenPorts.begin();
// 	for (; it != _listenPorts.end(); ++it) {
// 		const std::string& ip = it->first;
// 		int port = it->second;

// 		// Create the socket
// 		int listenFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
// 		if (listenFd < 0) {
// 			_webserv->ft_error("Socket creation error");
// 			return 1;
// 		}
// 		_webserv->printMsg("Server started", GREEN, "");

// 		// Set socket options
// 		int opt = 1;
// 		if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
// 			_webserv->ft_error("setsockopt() failed");
// 			close(listenFd);
// 			return 1;
// 		}

// 		// Set the address for the socket
// 		struct sockaddr_in addr;
// 		memset(&addr, 0, sizeof(addr));
// 		addr.sin_family = AF_INET;
// 		if (ip == "0.0.0.0")
// 			_addr.sin_addr.s_addr = INADDR_ANY;
// 		else {
// 			if (inet_pton(AF_INET, ip.c_str(), &_addr.sin_addr) <= 0) {
// 				std::cerr << "Invalid IP address: " << ip << std::endl;
// 				return 1;
// 			}
// 		}
// 		addr.sin_port = htons(port);

// 		// Bind the socket
// 		if (bind(listenFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
// 			_webserv->ft_error("bind() failed");
// 			close(listenFd);
// 			return 1;
// 		}

// 		// Listen for incoming connections
// 		if (listen(listenFd, SOMAXCONN) < 0) {
// 			_webserv->ft_error("listen() failed");
// 			close(listenFd);
// 			return 1;
// 		}

// 		// Store the file descriptor
// 		_fds.push_back(listenFd);

// 		std::cout << "Listening on " << ip << ":" << port << " (fd = " << listenFd << ")\n";
// 	}
// 	return 0;
// }


