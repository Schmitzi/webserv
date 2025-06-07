#include "../include/Webserv.hpp"

// Webserv::Webserv() : _epollFd(-1) { 
//     _confParser = ConfigParser("config/default.conf");
// }

Webserv::Webserv(std::string const &config) : _epollFd(-1) {
	_confParser = ConfigParser(config);
    _configs = _confParser.getAllConfigs();
    for (size_t i = 0; i < _configs.size(); i++) {//CHECK ALL CONFIGS
		bool toAdd = true;
		for (size_t j = 0; j < _servers.size(); j++) {//CHECK ALL SERVERS FOR CURRENT CONFIG FILE PORT
            std::vector<serverLevel>& servConfigs = _servers[j].getConfigs();
            for (size_t k = 0; k < servConfigs.size(); k++) {
                std::pair<std::pair<std::string, int>, bool> one = _confParser.getDefaultPortPair(_confParser.getConfigByIndex(i));
                std::pair<std::pair<std::string, int>, bool> two = _confParser.getDefaultPortPair(servConfigs[k]);
				if (one.first.first == two.first.first && one.first.second == two.first.second) {
					toAdd = false;
					break;
				}
            }
        }
		if (toAdd)
			_servers.push_back(Server(_confParser, i, *this));
	}
}

Webserv::Webserv(Webserv const &other) : _epollFd(-1) {
    *this = other;
}

Webserv &Webserv::operator=(Webserv const &other) {
    if (this != &other) {
        cleanup();
		for (size_t i = 0; i < other._servers.size(); i++)
			_servers.push_back(other._servers[i]);
		for (size_t i = 0; i < other._clients.size(); i++)
			_clients.push_back(other._clients[i]);
		_env = other._env;
		_confParser = other._confParser;
		for (size_t i = 0; i < other._configs.size(); i++)
			_configs.push_back(other._configs[i]);
        _epollFd = other._epollFd;
	}
	return *this;
}

Webserv::~Webserv() {
    cleanup();
}

// Server &Webserv::getServer(int i) {
//     return _servers[i];
// }

// ConfigParser &Webserv::getConfigParser() {
// 	return _confParser;
// }

void Webserv::setEnvironment(char **envp) {
    _env = envp;
}

// char **Webserv::getEnvironment() const {
//     return _env;
// }

// int Webserv::setConfig(std::string const &filepath) {
//     std::cout << GREEN << getTimeStamp() << "Config found at " << RESET << filepath << "\n";
// 	_confParser = ConfigParser(filepath);
// 	_configs = _confParser.getAllConfigs();
//     return true;
// }

// serverLevel &Webserv::getDefaultConfig() {
// 	for (size_t i = 0; i < _servers.size(); i++) {
// 		serverLevel conf = _servers[i].getConfigs()[i];
// 		for (size_t j = 0; j < conf.port.size(); j++) {
// 			if (conf.port[j].second == true)
// 				return _servers[i].getConfigs()[i];
// 		}
// 	}
// 	return _servers[0].getConfigs()[0];
// }

int Webserv::run() {
    _epollFd = epoll_create1(EPOLL_CLOEXEC);
    if (_epollFd == -1) {
        ft_error("epoll_create1() failed");
        return 1;
    }
   // Initialize server
   for (size_t i = 0; i < _servers.size(); i++) {
    if (_servers[i].getFd() > 0) {
        std::cout << BLUE << getTimeStamp() << "Host:Port already opened: " << RESET << 
            _confParser.getDefaultPortPair(_servers[i].getConfigs()[0]).first.first << ":" << 
            _confParser.getDefaultPortPair(_servers[i].getConfigs()[0]).first.second << "\n";
        i++;
        continue;
    }
    std::cout << BLUE << getTimeStamp() << "Initializing server " << i + 1 << " with port " << RESET << _confParser.getPort(_servers[i].getConfigs()[0]) << std::endl;
    if (_servers[i].openSocket() || _servers[i].setOptional() || 
        _servers[i].setServerAddr() || _servers[i].ft_bind() || _servers[i].ft_listen()) {
        std::cerr << RED << getTimeStamp() << "Failed to initialize server: " << RESET << i + 1 << std::endl;
        continue;
    }
    if (addToEpoll(_servers[i].getFd(), EPOLLIN) != 0) {
        std::cerr << RED << getTimeStamp() << "Failed to add server to epoll: " << RESET << i + 1 << std::endl;
        continue;
    }
    std::cout << GREEN << getTimeStamp() << 
        "Server " << i + 1 << " is listening on port " << RESET << 
        _confParser.getPort(_servers[i].getConfigs()[0]) << "\n";
    }
    while (1) {
        int nfds = epoll_wait(_epollFd, _events, MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR) {
                continue;
            }
            ft_error("epoll_wait() failed");
            continue;
        }
        for (int i = 0; i < nfds; i++) {
			int fd = _events[i].data.fd;
            uint32_t eventMask = _events[i].events;
            
            if (eventMask & EPOLLHUP) {
                std::cout << BLUE << getTimeStamp() << "Client disconnected: fd " << fd << RESET << std::endl;
                handleClientDisconnect(fd);
                continue;
            }
            if (eventMask & EPOLLERR) {
                std::cerr << RED << getTimeStamp() << "Socket error on fd " << fd << RESET << std::endl;
                handleErrorEvent(fd);
                continue;
            }
			bool found = false;
            Server activeServer = findServerByFd(fd, found);
            if (found) {
                if (eventMask & EPOLLIN) {
                    handleNewConnection(activeServer);
                }
            } else {
                if (eventMask & EPOLLIN) {
                    handleClientActivity(fd);
                }
            }
        }
    }
    return 0;
}

Server Webserv::findServerByFd(int fd, bool& found) {
    for (size_t i = 0; i < _servers.size(); i++) {
        if (_servers[i].getFd() == fd) {
			found = true;
			return _servers[i];
        }
    }
	if (!_servers.empty())
		return _servers[0];
	throw configException("Can't find server by FD");
}

Client Webserv::findClientByFd(int fd, bool& found) {
    for (size_t i = 0; i < _clients.size(); i++) {
        if (_clients[i].getFd() == fd) {
			found = true;
            return _clients[i];
        }
    }
	if (!_clients.empty())
		return _clients[0];
    throw configException("Can't find client by FD");
}

void Webserv::handleErrorEvent(int fd) {
    removeFromEpoll(fd);

    for (size_t i = 0; i < _clients.size(); i++) {
        if (_clients[i].getFd() == fd) {
            std::cout << RED << getTimeStamp() << "Removing client due to error: " << fd << RESET << std::endl;
            
            close(_clients[i].getFd());
            _clients.erase(_clients.begin() + i);
            return;
        }
    }
    
    std::cerr << RED << getTimeStamp() << "Error on unknown fd: " << fd << RESET << std::endl;
}

int Webserv::addToEpoll(int fd, short events) {  
    struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &event) == -1) {
        ft_error("epoll_ctl ADD failed for fd " + tostring(fd));
        return 1;
    }
    return 0;
}

void Webserv::removeFromEpoll(int fd) {
    if (_epollFd < 0 || fd < 0) {
        return;
    }
    
    if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        if (errno != EBADF && errno != ENOENT) {
            std::cerr << "Warning: epoll_ctl DEL failed for fd " << fd << ": " << strerror(errno) << std::endl;
        }
    }
}

void Webserv::handleClientDisconnect(int fd) {
    removeFromEpoll(fd);

    for (size_t i = 0; i < _clients.size(); i++) {
        if (_clients[i].getFd() == fd) {
            std::cout << GREEN << getTimeStamp() << "Cleaned up client connection: " << fd << RESET << std::endl;
            
            close(_clients[i].getFd());
            _clients.erase(_clients.begin() + i);
            return;
        }
    }
    
    std::cerr << RED << getTimeStamp() << "Disconnect on unknown fd: " << fd << RESET << std::endl;
}

void Webserv::handleNewConnection(Server &server) {
	if (_clients.size() >= 1000) { // Adjust limit as needed
        std::cerr << "Connection limit reached, refusing new connection" << std::endl;
        
        struct sockaddr_in addr;
        socklen_t addrLen = sizeof(addr);
        int newFd = accept(server.getFd(), (struct sockaddr *)&addr, &addrLen);
        if (newFd >= 0) {
            close(newFd);
        }
        return;
    }
    Client newClient = Client(server);
    
    if (newClient.acceptConnection(server.getFd()) == 0) {
        newClient.displayConnection();

        if (addToEpoll(newClient.getFd(), EPOLLIN) == 0) {
            _clients.push_back(newClient);
        } else {
            printMsg("Failed to add client to epoll", RED, "");
            close(newClient.getFd());
        }
    }
}

void Webserv::handleClientActivity(int clientFd) {
	std::cout << BLUE << getTimeStamp() << "Handling activity for fd: " << clientFd << RESET << std::endl;
    
    // Find the client properly
    Client* clientPtr = NULL;
    for (size_t i = 0; i < _clients.size(); i++) {
        if (_clients[i].getFd() == clientFd) {
            clientPtr = &_clients[i];
            break;
        }
    }
    
    if (!clientPtr) {
        std::cerr << RED << getTimeStamp() << "Client not found for fd: " << clientFd << RESET << std::endl;
        removeFromEpoll(clientFd);
        close(clientFd);
        return;
    }
    
    std::cout << BLUE << getTimeStamp() << "Found client with fd: " << clientPtr->getFd() << RESET << std::endl;
    
    if (clientPtr->recieveData() != 0) {
        removeFromEpoll(clientFd);
        close(clientFd);

        for (size_t i = 0; i < _clients.size(); i++) {
            if (_clients[i].getFd() == clientFd) {
                _clients.erase(_clients.begin() + i);
                break;
            }
        }
    }
}

void    Webserv::ft_error(std::string const msg) {
    printMsg("Error: " + msg, RED, strerror(errno));
}

void    Webserv::printMsg(const std::string msg, char const *colour, std::string const opt) {
    if (opt.empty()) {
        std::cout << colour << getTimeStamp() << msg << RESET << "\n";
    } else {
        std::cout << colour << getTimeStamp() << msg << ": " << RESET << opt << "\n"; //_address.sin_port
    }
}

std::string Webserv::getTimeStamp() {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    std::ostringstream oss;
    oss << "[" 
        << (tm_info->tm_year + 1900) << "-"
        << std::setw(2) << std::setfill('0') << (tm_info->tm_mon + 1) << "-"
        << std::setw(2) << std::setfill('0') << tm_info->tm_mday << " "
        << std::setw(2) << std::setfill('0') << tm_info->tm_hour << ":"
        << std::setw(2) << std::setfill('0') << tm_info->tm_min << ":"
        << std::setw(2) << std::setfill('0') << tm_info->tm_sec << "] ";
    
    return oss.str();
}

void    Webserv::cleanup() {
    for (size_t i = 0; i < _clients.size(); ++i) {
        if (_clients[i].getFd() >= 0) {
            removeFromEpoll(_clients[i].getFd());
            close(_clients[i].getFd());
        }
    }
    // _clients.clear();

    for (size_t i = 0; i < _servers.size(); i++) {
        if (_servers[i].getFd() >= 0) {
            removeFromEpoll(_servers[i].getFd());
            close(_servers[i].getFd());
        }

    }

    _servers.clear();

    if (_epollFd >= 0) {
        close(_epollFd);
        _epollFd = -1;
    }
}
