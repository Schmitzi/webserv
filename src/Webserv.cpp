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

void Webserv::addSendBuf(int fd, const std::string& s) {
	_sendBuf[fd] += s;
}

const std::string& Webserv::getSendBuf(int fd) {
	return _sendBuf[fd];
}

void Webserv::clearSendBuf(int fd) {
	_sendBuf.erase(fd);
}

bool Webserv::isCgiPipeFd(int fd) const {
    return _cgis.find(fd) != _cgis.end();
}

void Webserv::registerCgiPipe(int fd, CGIHandler* handler) {
    _cgis[fd] = handler;
}

CGIHandler* Webserv::getCgiHandler(int fd) const {
    std::map<int, CGIHandler*>::const_iterator it = _cgis.find(fd);
    if (it != _cgis.end())
        return it->second;
    return NULL;
}

void Webserv::unregisterCgiPipe(int fd) {
    std::map<int, CGIHandler*>::iterator it = _cgis.find(fd);
    if (it != _cgis.end())
        _cgis.erase(it); 
}

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

bool Webserv::checkEventMaskErrors(uint32_t &eventMask, int &fd) {
    if (eventMask & EPOLLHUP) {
        if (isCgiPipeFd(fd)) {
            std::cout << getTimeStamp(fd) << BLUE << "CGI pipe received EPOLLHUP (child closed pipe)" << RESET << std::endl;
            return true;
        } else {
            std::cout << getTimeStamp(fd) << YELLOW << "Client hangup detected" << RESET << std::endl;
            handleClientDisconnect(fd);
            return false;
        }
    }
    
    if (eventMask & EPOLLERR) {
        std::cerr << getTimeStamp(fd) << RED << "Socket error on FD: " << fd << RESET << std::endl;
        if (isCgiPipeFd(fd)) {
            std::cout << getTimeStamp(fd) << RED << "CGI pipe error" << RESET << std::endl;
            CGIHandler* handler = getCgiHandler(fd);
            if (handler) {
                handler->cleanupResources();
                delete handler;
            }
        } else {
            handleErrorEvent(fd);
        }
        return false;
    }
    
    return true;
}

int Webserv::getEpollFd() {
	return _epollFd;
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
            
            if (!checkEventMaskErrors(eventMask, fd))
                continue;
            
            Server *activeServer = findServerByFd(fd);
            
            if (activeServer) {
                if (eventMask & EPOLLIN)
                    handleNewConnection(*activeServer, eventMask);
            } else {
                if (eventMask & (EPOLLIN | EPOLLHUP))
                    handleClientActivity(fd);
                    
                if (eventMask & EPOLLOUT)
                    handleEpollOut(fd);
            }
        }
    }
    return 0;
}

Server* Webserv::findServerByFd(int fd) {
    for (size_t i = 0; i < _servers.size(); i++) {
        if (_servers[i].getFd() == fd) {
			return &_servers[i];
        }
    }
	return NULL;
}

Client* Webserv::findClientByFd(int fd) {
	for (size_t i = 0; i < _clients.size(); i++) {
		if (_clients[i].getFd() == fd) {
			return &_clients[i];
		}
	}
	return NULL;
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

void Webserv::setEpollEvents(int fd, uint32_t events) {
	if (fd < 0)
		return;
	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd;
	if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev) == -1)
		std::cerr << getTimeStamp(fd) << RED << "Error: epoll_ctl MOD failed" << RESET << std::endl;
}

int Webserv::addToEpoll(int fd, short events) {  
    struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &event) == -1) {
		std::cerr << getTimeStamp(fd) << RED << "Error: epoll_ctl ADD failed" << RESET << std::endl;
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
	std::cout << getTimeStamp(fd) << "Client disconnected\n";
}

void Webserv::handleClientDisconnect(int fd) {
    std::cout << getTimeStamp(fd) << YELLOW << "Handling client disconnect for fd: " << fd << RESET << std::endl;
    
    std::vector<int> cgiPipesToCleanup;
    for (std::map<int, CGIHandler*>::iterator it = _cgis.begin(); it != _cgis.end(); ++it) {
        CGIHandler* handler = it->second;
        if (handler && handler->getClient() && handler->getClient()->getFd() == fd) {
            std::cout << getTimeStamp(fd) << YELLOW << "Found CGI handler for disconnected client, cleaning up" << RESET << std::endl;
            cgiPipesToCleanup.push_back(it->first);
        }
    }
    
    for (size_t i = 0; i < cgiPipesToCleanup.size(); ++i) {
        int cgiFd = cgiPipesToCleanup[i];
        CGIHandler* handler = getCgiHandler(cgiFd);
        if (handler) {
            handler->cleanupResources();
            delete handler;
        }
        unregisterCgiPipe(cgiFd);
    }
    
    removeFromEpoll(fd);

    for (size_t i = 0; i < _clients.size(); i++) {
        if (_clients[i].getFd() == fd) {
            std::cout << getTimeStamp(fd) << GREEN << "Cleaned up client connection" << RESET << std::endl;
            close(_clients[i].getFd());
            _clients.erase(_clients.begin() + i);
            return;
        }
    }
    std::cerr << getTimeStamp(fd) << RED << "Disconnect on unknown fd: " << fd << RESET << std::endl;
}

void Webserv::handleNewConnection(Server &server, u_int32_t eventMask) {
	if (_clients.size() >= 1000) {
        std::cerr << getTimeStamp() << RED << "Connection limit reached, refusing new connection" << RESET << std::endl;
        
        struct sockaddr_in addr;
        socklen_t addrLen = sizeof(addr);
        int newFd = accept(server.getFd(), (struct sockaddr *)&addr, &addrLen);
        if (newFd >= 0) {
            std::cout << RED << "New connection failed\n" << RESET;
            close(newFd);
        }
        return;
    }
    Client newClient(server, eventMask);    
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
    if (isCgiPipeFd(clientFd)) {
        std::cout << getTimeStamp(clientFd) << BLUE << "Handling CGI pipe activity for FD: " << clientFd << RESET << std::endl;
        CGIHandler* handler = getCgiHandler(clientFd);
        if (handler) {
            int result = handler->processScriptOutput();
            if (result == 1) {
                std::cout << getTimeStamp(clientFd) << GREEN << "CGI processing completed for FD: " << clientFd << RESET << std::endl;
                
                handler->cleanupResources();
                unregisterCgiPipe(clientFd);
                delete handler;
            } else if (result == 0) {
                std::cout << getTimeStamp(clientFd) << BLUE << "CGI still processing for FD: " << clientFd << RESET << std::endl;
            } else {
                std::cout << getTimeStamp(clientFd) << RED << "CGI processing error for FD: " << clientFd << RESET << std::endl;
                
                handler->cleanupResources();
                unregisterCgiPipe(clientFd); 
                delete handler;
            }
        } else {
            std::cerr << getTimeStamp(clientFd) << RED << "No CGI handler found for FD: " << clientFd << RESET << std::endl;
        }
        return;
    }
    Client *client = findClientByFd(clientFd);
    
    if (!client) {
        std::cerr << getTimeStamp(clientFd) << RED << "Client not found for FD: " << clientFd << RESET << std::endl;
        removeFromEpoll(clientFd);
        close(clientFd);
        return;
    }
    
    client->recieveData();
}

void Webserv::handleEpollOut(int fd) {
	Client *c = findClientByFd(fd);
	if (!c) {
		std::cerr << getTimeStamp(fd) << RED << "Error: no client was found" << RESET << std::endl;
		return;
	}

	const std::string& toSend = getSendBuf(fd);
	if (toSend.empty()) {
		std::cerr << getTimeStamp(fd) << RED << "Error: no _sendBuf was stored" << RESET << std::endl;
		return;
	}

	size_t& offset = c->getOffset();
	const char* data = toSend.data() + offset;
	size_t remaining = toSend.size() - offset;

	ssize_t s = send(fd, data, remaining, 0);
	if (s < 0) {
		std::cerr << getTimeStamp(fd) << RED << "Error: send() failed" << RESET << std::endl;
		clearSendBuf(fd);
		c->setExitCode(1);
	}

	offset += static_cast<size_t>(s);

	if (offset >= toSend.size()) {
		offset = 0;
		clearSendBuf(fd);
		c->setExitCode(1);
	}
	if (c->getExitCode() != 0) {
		handleClientDisconnect(fd);
	}
}

void    Webserv::cleanup() {
    std::cout << "\r" << std::string(80, ' ') << "\r" << std::flush;
    std::cout << CYAN << "Received signal, cleaning up...\n" << RESET;
    for (std::map<int, CGIHandler*>::iterator it = _cgis.begin(); it != _cgis.end(); ++it) {
        if (it->second) {
            it->second->cleanupResources();
            delete it->second;
        }
    }
    _cgis.clear();
    
    for (size_t i = 0; i < _clients.size(); ++i) {
        if (_clients[i].getFd() >= 0) {
            removeFromEpoll(_clients[i].getFd());
            close(_clients[i].getFd());
        }
    }
    _clients.clear();

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
    std::cout << CYAN << "Goodbye!" << RESET << std::endl;
}
