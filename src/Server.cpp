#include "../include/Server.hpp"
#include "../include/Webserv.hpp"

Server::Server() : _uploadDir("local/upload/"), _webRoot("local"), _webserv(NULL) {

}

Server  &Server::operator=(Server const &other) {
    if (this != &other) {
        _fd = other._fd;
        _addr = other._addr;
        _uploadDir = other._uploadDir;
        _webRoot = other._webRoot;
        _config = other._config;
        _webserv = other._webserv;
        for (size_t i = 0; i < other._pfds.size(); i++)
            _pfds.push_back(other._pfds[i]);
    }
    return *this;
}

Server::~Server()  {
    _pfds.clear();
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

std::string const &Server::getUploadDir() {
    return _uploadDir;
}

std::string const   &Server::getWebRoot() {
    return _webRoot;
}

std::vector<struct pollfd>  &Server::getPfds() {
    return _pfds;
}

void    Server::addPfd(struct pollfd newPfd) {
    _pfds.push_back(newPfd);
}

void Server::setWebserv(Webserv* webserv) {
    _webserv = webserv;
	std::string root = webserv->getConfig().getConfig().rootServ;
	if (!root.empty())
		_webRoot = root;
	std::string upload = webserv->getConfig().getConfig().locations["/upload"].uploadDirPath;
	if (!upload.empty())
		_uploadDir = upload;
	//TODO: use all the locations-> each one can have a different rootLoc, uploadDirPath etc.
}

void    Server::setConfig(Config config) {
    _config = config;
}

void    Server::setFd(int const fd) {
    _fd = fd;
}

void    Server::removePfd(int index) {
    _pfds.erase(_pfds.begin() + index);
}

// Add a file descriptor to the poll array
int Server::addToPoll(int fd, short events) {  
    // Add to poll array
    struct pollfd temp;
    temp.fd = fd;
    temp.events = events;
    temp.revents = 0;
    addPfd(temp);
    
    return 0;
}

// Remove a file descriptor from the poll array by index
void Server::removeFromPoll(size_t index) {
    if (index >= getPfds().size()) {
        getWebServ().printMsg("Invalid poll index", RED, "");
        return;
    }
    removePfd(index);
}

int Server::openSocket() { // Create a TCP socket
    _fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
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


