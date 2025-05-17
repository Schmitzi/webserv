#include "../include/Server.hpp"
#include "../include/Webserv.hpp"

Server::Server(ConfigParser confs, int nbr) {
	_config = Config(confs, nbr);
	serverLevel conf = _config.getConfig();
	if (!conf.rootServ.empty())
		_webRoot = conf.rootServ;//TODO: what about locations?
	else
		_webRoot = "local";
	std::map<std::string, locationLevel>::iterator it = conf.locations.begin();//TODO: just gets first uploadPath, which one should it actually get?
	for (; it != conf.locations.end(); ++it) {
		if (!it->second.uploadDirPath.empty()) {
			_uploadDir = it->second.uploadDirPath;
            if (_uploadDir[strlen(_uploadDir.c_str())] != '/') {
                _uploadDir += '/';
            }
			break;
		}
	}
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

std::string const &Server::getUploadDir() {
    return _uploadDir;
}

std::string const   &Server::getWebRoot() {
    return _webRoot;
}

Config &Server::getConfigClass() {
	return _config;
}

void Server::setWebserv(Webserv* webserv) {
    _webserv = webserv;
}

void    Server::setConfig(Config config) {
    _config = config;
}

void    Server::setFd(int const fd) {
    _fd = fd;
}

int Server::openSocket() {
    // Create a TCP socket
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

int Server::setServerAddr() {
    memset(&_addr, 0, sizeof(_addr));
    _addr.sin_family = AF_INET;
    
    std::pair<std::pair<std::string, int>, bool> conf = getConfigClass().getDefaultPortPair();
    std::string ip = conf.first.first;
    int port = conf.first.second;
    
    // Convert string IP to network format
    if (ip == "0.0.0.0" || ip.empty()) {
        _addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, ip.c_str(), &(_addr.sin_addr));
    }
    
    _addr.sin_port = htons(port);
    
    std::cout << GREEN << _webserv->getTimeStamp() << "Server binding to " << RESET << ip << ":" << port << std::endl;
    
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