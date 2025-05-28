#include "../include/Server.hpp"
#include "../include/Webserv.hpp"

Server::Server(ConfigParser confs, int nbr, Webserv& webserv) {
	_fd = -1;
	_confParser = confs;
    // _curConfig = confs.getConfigByIndex(nbr);
	std::pair<std::pair<std::string, int>, bool> portPair = confs.getDefaultPortPair(confs.getConfigByIndex(nbr));
	IPPortToServersMap temp = confs.getIpPortToServers();
	IPPortToServersMap::iterator it = temp.begin();
    for (; it != confs.getIpPortToServers().end(); ++it) {
		if (it->first == portPair) {
			_configs = it->second;
			break;
		}
	}
	_webserv = &webserv;
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

// serverLevel &Server::getCurConfig() {
// 	return _curConfig;
// }

std::vector<serverLevel> &Server::getConfigs() {
	return _configs;
}

ConfigParser &Server::getConfParser() {
	return _confParser;
}

std::string Server::getUploadDir(Client& client, Request& req) {
	locationLevel loc;
	if (!matchUploadLocation(req.getReqPath(), req.getConf(), loc)) {
		std::cout << "Location not found: " << req.getReqPath() << std::endl;
		client.sendErrorResponse(403, req);
		return "";
	}
	if (loc.uploadDirPath.empty()) {
		std::cout << "Upload directory not set: " << req.getReqPath() << std::endl;
		client.sendErrorResponse(403, req);
		return "";
	}
    std::string fullPath = getWebRoot(req, loc) + req.getPath();
	return fullPath;
}

std::string	Server::getWebRoot(Request& req, locationLevel& loc) {
	std::string path;
	if (!loc.rootLoc.empty()) {
		if (loc.rootLoc[loc.rootLoc.size() - 1] == '/')
			path = loc.rootLoc.substr(0, loc.rootLoc.size() - 1);
		else
			path = loc.rootLoc;
	}
	else {
		if (req.getConf().rootServ[req.getConf().rootServ.size() - 1] == '/')
			path = req.getConf().rootServ.substr(0, req.getConf().rootServ.size() - 1);
		else
			path = req.getConf().rootServ;
	}
	return path;
}

Config &Server::getConfigClass() {
	return _curConfig;
}

std::vector<serverLevel*> &Server::getConfigs() {
	return _configs;
}

void Server::setWebserv(Webserv* webserv) {
    _webserv = webserv;
}

void    Server::setConfigs(std::vector<serverLevel> configs) {
    _configs = configs;
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
        _webserv->ft_error("setsockopt(SO_REUSEADDR) failed");
        close(_fd);
        return 1;
    }
    
    return 0;
}

int Server::setServerAddr() {
    memset(&_addr, 0, sizeof(_addr));
    _addr.sin_family = AF_INET;
    
    std::pair<std::pair<std::string, int>, bool> conf = _confParser.getDefaultPortPair(getConfigs()[0]);
    std::string ip = conf.first.first;
    int port = conf.first.second;
    
    // Convert string IP to network format
    if (ip == "0.0.0.0" || ip.empty()) {
        _addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, ip.c_str(), &(_addr.sin_addr));
    }
//     std::tuple<std::string, std::string, std::string> hostPortName;
//    _curConfig.getConfig().servName; 
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
    const int backlog = 128;
    
    if (listen(_fd, backlog) < 0) {
        _webserv->ft_error("listen() failed");
        close(_fd);
        return 1;
    }
    return 0;
}
