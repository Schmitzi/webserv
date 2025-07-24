#include "../include/Server.hpp"
#include "../include/Webserv.hpp"
#include "../include/Client.hpp"
#include "../include/Request.hpp"
#include "../include/Response.hpp"
#include "../include/Helper.hpp"

Server::Server(ConfigParser confs, int nbr, Webserv& webserv) {
	_fd = -1;
	_confParser = confs;
	
	std::pair<std::pair<std::string, int>, bool> portPair = confs.getDefaultPortPair(confs.getConfigByIndex(nbr));
	std::string targetIp = portPair.first.first;
	int targetPort = portPair.first.second;
	_configs.clear();
	
	std::vector<serverLevel> allConfigs = confs.getAllConfigs();
	for (size_t i = 0; i < allConfigs.size(); i++) {
		for (size_t j = 0; j < allConfigs[i].port.size(); j++) {
			std::string configIp = allConfigs[i].port[j].first.first;
			int configPort = allConfigs[i].port[j].first.second;
			
			if (configIp == targetIp && configPort == targetPort) {
				_configs.push_back(allConfigs[i]);
				break;
			}
		}
	}
	
	_webserv = &webserv;
}

Server::Server(const Server& copy) {
	*this = copy;
}

Server &Server::operator=(const Server& copy) {
	if (this != &copy) {
		_fd = copy._fd;
		_addr = copy._addr;
		_confParser = copy._confParser;
		_configs = copy._configs;
		_webserv = copy._webserv;
	}
	return *this;
}

Server::~Server()  {
	_configs.clear();
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

std::vector<serverLevel> &Server::getConfigs() {
	return _configs;
}

ConfigParser &Server::getConfParser() {
	return _confParser;
}

std::string Server::getUploadDir(Client& client, Request& req) {
	locationLevel* loc = NULL;
	if (!matchUploadLocation(req.getPath(), req.getConf(), loc)) {
		req.statusCode() = 404;
		client.output() = getTimeStamp(client.getFd()) + RED + "Location not found: " + RESET + req.getPath();
		sendErrorResponse(client, req);
		return "";
	}
	if (loc->uploadDirPath.empty()) {
		req.statusCode() = 403;
		client.output() = getTimeStamp(client.getFd()) + RED + "Upload directory not set: " + RESET + req.getPath();
		sendErrorResponse(client, req);
		return "";
	}
	std::string fullPath = matchAndAppendPath(getWebRoot(req, *loc), req.getPath());
	return fullPath;
}

std::string	Server::getWebRoot(Request& req, locationLevel& loc) {
	if (!loc.rootLoc.empty()) {
		if (loc.isRegex)
			return loc.rootLoc;
		std::string x = matchAndAppendPath(loc.rootLoc, loc.locName);
		return x;
	}
	return req.getConf().rootServ;
}

int Server::setOptional() { 
	int opt = 1;
	
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		std::cerr << getTimeStamp() << RED << "Error: setsockopt(SO_REUSEADDR) failed" << RESET << std::endl;
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
	
	if (ip == "0.0.0.0" || ip.empty())
		_addr.sin_addr.s_addr = INADDR_ANY;
	else
		inet_pton(AF_INET, ip.c_str(), &(_addr.sin_addr));
	_addr.sin_port = htons(port);
	
	std::cout << getTimeStamp() << GREEN << "Server binding to " << ip << ":" << port << RESET << std::endl;
	return 0;
}

int Server::openSocket() {
	_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (_fd < 0) {
		std::cerr << getTimeStamp() << RED << "Error: socket() failed" << RESET << std::endl;
		return 1;
	}
	std::cout << getTimeStamp() << GREEN << "Server started" << RESET << std::endl;
	return 0;
}

int Server::ft_bind() {
	if (bind(_fd, (struct sockaddr *)&_addr, sizeof(_addr)) < 0) {
		std::cerr << getTimeStamp() << RED << "Error: bind() failed" << RESET << std::endl;
		close(_fd);
		return 1;
	}
	return 0;
}

int Server::ft_listen() {
	const int backlog = 128;
	
	if (listen(_fd, backlog) < 0) {
		std::cerr << getTimeStamp() << RED << "Error: listen() failed" << RESET << std::endl;
		close(_fd);
		return 1;
	}
	return 0;
}
