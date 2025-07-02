#include "../include/Webserv.hpp"

Webserv::Webserv(std::string config) : _epollFd(-1) {
	_state = false;
	_confParser = ConfigParser(config);
    _configs = _confParser.getAllConfigs();
    for (size_t i = 0; i < _configs.size(); i++) {
		bool toAdd = true;
		for (size_t j = 0; j < _servers.size(); j++) {
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
    _state = true;
}

Webserv::Webserv(Webserv const &other) : _epollFd(-1) {
    *this = other;
}

Webserv &Webserv::operator=(Webserv const &other) {
    if (this != &other) {
        cleanup();
		_state = other._state;
		_servers = other._servers;
		_clients = other._clients;
		_env = other._env;
        _epollFd = other._epollFd;
		_confParser = other._confParser;
		_configs = other._configs;
	}
	return *this;
}

Webserv::~Webserv() {
    cleanup();
}

void Webserv::flipState() {
	_state = false;
}

void Webserv::setEnvironment(char **envp) {
    _env = envp;
}

// char	**Webserv::getEnv() {
// 	return _env;
// }

void Webserv::initialize() {
    for (size_t i = 0; i < _servers.size(); i++) {
    	if (_servers[i].getFd() > 0) {
			std::cout << getTimeStamp() << BLUE << "Host:Port already opened: " << RESET << 
				_confParser.getDefaultPortPair(_servers[i].getConfigs()[0]).first.first << ":" << 
				_confParser.getDefaultPortPair(_servers[i].getConfigs()[0]).first.second << std::endl;
			i++;
			continue;
		}
		std::cout << getTimeStamp() << BLUE << "Initializing server " << i + 1 << " with port " << _confParser.getPort(_servers[i].getConfigs()[0]) << RESET << std::endl;
		if (_servers[i].openSocket() || _servers[i].setOptional() || 
			_servers[i].setServerAddr() || _servers[i].ft_bind() || _servers[i].ft_listen()) {
			std::cerr << getTimeStamp() << RED << "Failed to initialize server: " << RESET << i + 1 << std::endl;
			continue;
		}
		if (addToEpoll(_servers[i].getFd(), EPOLLIN) != 0) {
			std::cerr << getTimeStamp() << RED << "Failed to add server to epoll: " << RESET << i + 1 << std::endl;
			continue;
		}
		std::cout << getTimeStamp() << GREEN << "Server " << i + 1;
		bool smth = false;
		for (size_t x = 0; x < _servers[i].getConfigs().size(); x++) {
			if (x == 0 && !_servers[i].getConfigs()[x].servName[0].empty()) {
				std::cout << " [";
				smth = true;
			}
			std::cout << _servers[i].getConfigs()[x].servName[0];
			if (x + 1 < _servers[i].getConfigs().size() && !_servers[i].getConfigs()[x].servName[0].empty())
				std::cout << ", ";
			else if (smth == true)
				std::cout << "]";
		}
		std::cout << " is listening on port " << _confParser.getPort(_servers[i].getConfigs()[0]) << RESET << std::endl << std::endl;
    }
}

int Webserv::run() {
    _epollFd = epoll_create1(EPOLL_CLOEXEC);
    if (_epollFd == -1) {
		std::cerr << getTimeStamp() << RED << "Error: epoll_create1() failed" << RESET << std::endl;
        return 1;
    }
    initialize();
    while (_state == true) {
        int nfds = epoll_wait(_epollFd, _events, MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR)
                continue;
            std::cerr << getTimeStamp() << RED << "Error: epoll_wait() failed" << RESET << std::endl;
            continue;
        }
        for (int i = 0; i < nfds; i++) {
			int fd = _events[i].data.fd;
            uint32_t eventMask = _events[i].events;
            
            if (eventMask & EPOLLHUP) {
                handleClientDisconnect(fd);
                continue;
            }
            if (eventMask & EPOLLERR) {
                std::cerr << getTimeStamp(fd) << RED << "Socket error" << RESET << std::endl;
                handleErrorEvent(fd);
                continue;
            }
			bool found = false;
            Server activeServer = findServerByFd(fd, found);
            if (found) {
                if (eventMask & EPOLLIN)
                    handleNewConnection(activeServer);
            } else {
                if (eventMask & EPOLLIN) {
                    handleClientActivity(fd);
					if (eventMask & EPOLLHUP) {
						handleClientDisconnect(fd);
						continue;
            		}
				}
                if (eventMask & EPOLLOUT)
                    std::cerr << RED << "WE NEED TO IMPLEMENT THIS" << RESET << std::endl;//TODO: Implement EPOLLOUT handling?
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

void Webserv::handleErrorEvent(int fd) {
    removeFromEpoll(fd);
    for (size_t i = 0; i < _clients.size(); i++) {
        if (_clients[i].getFd() == fd) {
            std::cerr << getTimeStamp(fd) << RED << "Removing client due to error " << RESET << std::endl;
            
            close(_clients[i].getFd());
            _clients.erase(_clients.begin() + i);
            return;
        }
    }
    std::cerr << getTimeStamp(fd) << RED << "Error on unknown fd" << RESET << std::endl;
}

int Webserv::addToEpoll(int fd, short events) {  
    struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &event) == -1) {
		std::cerr << getTimeStamp() << RED << "Error: epoll_ctl ADD for fd " << tostring(fd) << " failed" << RESET << std::endl;
        return 1;
    }
    return 0;
}

void Webserv::removeFromEpoll(int fd) {
	if (_epollFd < 0 || fd < 0)
		return;
    if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) == -1) {
		if (errno != EBADF && errno != ENOENT)
		std::cerr << getTimeStamp(fd) << RED << "Warning: epoll_ctl DEL failed" << RESET << std::endl;
    }
	std::cout << getTimeStamp(fd) << "Client disconnected" << std::endl;
}

void Webserv::handleClientDisconnect(int fd) {
    removeFromEpoll(fd);

    for (size_t i = 0; i < _clients.size(); i++) {
        if (_clients[i].getFd() == fd) {
            std::cout << getTimeStamp(fd) << GREEN << "Cleaned up client connection" << RESET << std::endl;
            
            close(_clients[i].getFd());
            _clients.erase(_clients.begin() + i);
            return;
        }
    }
    std::cerr << getTimeStamp(fd) << RED << "Disconnect on unknown fd" << RESET << std::endl;
}

void Webserv::handleNewConnection(Server &server) {
	// std::cout << "================================================================================" << std::endl;
	if (_clients.size() >= 1000) { // Adjust limit as needed
        std::cerr << getTimeStamp() << RED << "Connection limit reached, refusing new connection" << RESET << std::endl;
        
        struct sockaddr_in addr;
        socklen_t addrLen = sizeof(addr);
        int newFd = accept(server.getFd(), (struct sockaddr *)&addr, &addrLen);
        if (newFd >= 0)
            close(newFd);
        return;
    }
    Client newClient(server);
    
    if (newClient.acceptConnection(server.getFd()) == 0) {
        newClient.displayConnection();

        if (addToEpoll(newClient.getFd(), EPOLLIN) == 0)
            _clients.push_back(newClient);
        else {
			std::cerr << getTimeStamp(newClient.getFd()) << RED << "Failed to add client to epoll" << RESET << std::endl;
            close(newClient.getFd());
        }
    }
}

void Webserv::handleClientActivity(int clientFd) {    
    Client* clientPtr = NULL;
    for (size_t i = 0; i < _clients.size(); i++) {
        if (_clients[i].getFd() == clientFd) {
            clientPtr = &_clients[i];
            break;
        }
    }
    
    if (!clientPtr) {
        std::cerr << getTimeStamp(clientFd) << RED << "Client not found" << RESET << std::endl;
        removeFromEpoll(clientFd);
        close(clientFd);
        return;
    }
    
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

void    Webserv::cleanup() {
    for (size_t i = 0; i < _clients.size(); ++i) {
        if (_clients[i].getFd() >= 0) {
            removeFromEpoll(_clients[i].getFd());
            close(_clients[i].getFd());
        }
    }

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
