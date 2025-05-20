#include "../include/Server.hpp"
#include "../include/Webserv.hpp"

Server::Server(ConfigParser confs, int nbr) {
	_config = Config(confs, nbr);
	serverLevel conf = _config.getConfig();
	if (!conf.rootServ.empty())
		_webRoot = conf.rootServ;//TODO: what about locations?
	if (_webRoot.empty()) {
		//check locations
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

std::string Server::getUploadDir(Client& client, Request& req) {
	locationLevel loc;
	if (!matchUploadLocation(req.getReqPath(), _config.getConfig(), loc)) {
		std::cout << "Location not found: " << req.getReqPath() << std::endl;
		client.sendErrorResponse(403);
		return "";
	}
	if (loc.uploadDirPath.empty()) {
		std::cout << "Upload directory not set: " << req.getReqPath() << std::endl;
		client.sendErrorResponse(403);
		return "";
	}
    std::string fullPath = getWebRoot() + req.getReqPath();
	return fullPath;
}

std::string const   &Server::getWebRoot() {
	if (_webRoot.empty()) {
		
	}
    return _webRoot;
}

Config &Server::getConfigClass() {
	return _config;
}

void Server::setWebserv(Webserv* webserv) {
    _webserv = webserv;
	//TODO: use all the locations-> each one can have a different rootLoc, uploadDirPath etc.
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
