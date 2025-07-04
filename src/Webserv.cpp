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

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ GETTERS & SETTERS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void Webserv::setEnvironment(char **envp) {
	_env = envp;
}

void Webserv::flipState() {
	_state = false;
}

Server* Webserv::getServerByFd(int fd) {
    for (size_t i = 0; i < _servers.size(); i++) {
        if (_servers[i].getFd() == fd)
			return &_servers[i];
    }
	return NULL;
}

Client* Webserv::getClientByFd(int fd) {
	for (size_t i = 0; i < _clients.size(); i++) {
		if (_clients[i].getFd() == fd)
			return &_clients[i];
	}
	return NULL;
}

int Webserv::getEpollFd() {
	return _epollFd;
}

std::string& Webserv::getSendBuf(int fd) {
	return _sendBuf[fd];
}

std::map<int, std::string> &Webserv::getSendBuf() {
	return _sendBuf;
}

std::map<int, CGIHandler*> &Webserv::getCgis() {
	return _cgis;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

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
		if (addToEpoll(*this, _servers[i].getFd(), EPOLLIN) != 0) {
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
			// _events[i].events = EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLERR;
			
			if (eventMask & EPOLLERR) {
				std::cout << MAGENTA << "---> EPOLLERR <---" << RESET << std::endl;
				std::cerr << getTimeStamp(fd) << RED << "Error: Socket (EPOLLERR)" << RESET << std::endl;
				handleErrorEvent(fd);
				continue;
			}

			if (eventMask & EPOLLIN || eventMask & EPOLLHUP) {
				std::cout << MAGENTA << "---> EPOLLIN <---" << RESET << std::endl;
				Server* activeServer = getServerByFd(fd);
				if (activeServer) {
					std::cout << YELLOW << "~~~ NEW CONNECTION ~~~" << RESET << std::endl;
					handleNewConnection(*activeServer);
				}
				else {
					if (eventMask & EPOLLIN) {
						std::cout << YELLOW << "~~~ CLIENT ACTIVITY ~~~" << RESET << std::endl;
						handleClientActivity(fd);
					}
					if (isCgiPipeFd(*this, fd)) {
						std::cout << YELLOW << "~~~ CGI PIPE ~~~" << RESET << std::endl;
						CGIHandler* handler = getCgiHandler(*this, fd);
						if (handler && handler->handleCgiPipeEvent(fd, eventMask) != 0)
							std::cerr << getTimeStamp(fd) << RED << "Error: CGI processing failed" << RESET << std::endl;
						// continue;
					}
				}
			}

			if (eventMask & EPOLLOUT) {
				std::cout << MAGENTA << "---> EPOLLOUT <---" << RESET << std::endl;
				handleEpollOut(fd);
			}
		}
    }
    return 0;
}

void Webserv::handleEpollOut(int fd) {
	Client* c = getClientByFd(fd);
	if (!c) {
		std::cerr << getTimeStamp(fd) << RED << "Error: no client was found" << RESET << std::endl;
		return;
	}

	std::string& toSend = getSendBuf(fd);
	if (toSend.empty()) {
		std::cerr << getTimeStamp(fd) << RED << "Error: no _sendBuf was stored" << RESET << std::endl;
		return;
	}

	size_t& offset = c->getOffset();
	const char* data = toSend.data() + offset;
	size_t remaining = toSend.size() - offset;

	ssize_t s = send(fd, data, remaining, 0);
	if (s == 0) {
		std::cerr << getTimeStamp(fd) << RED << "Error: send() returned 0 (no data sent)" << RESET << std::endl;
		clearSendBuf(*this, fd);
		c->setExitCode(1);
		return;
	}
	if (s < 0) {
		std::cerr << getTimeStamp(fd) << RED << "Error: send() failed" << RESET << std::endl;
		clearSendBuf(*this, fd);
		c->setExitCode(1);
	}

	offset += static_cast<size_t>(s);

	if (offset >= toSend.size()) {
		offset = 0;
		clearSendBuf(*this, fd);
		if (c->getConnect() == "keep-alive") {
			if (setEpollEvents(*this, fd, EPOLLIN)) {
				std::cerr << getTimeStamp(fd) << RED << "Error: setEpollEvents() failed 10" << RESET << std::endl;
				return;
			}
		}
		else
			c->setExitCode(1);
	}
	if (c->getExitCode() != 0)
		handleClientDisconnect(fd);
}

void Webserv::handleNewConnection(Server &server) {
	if (_clients.size() >= 1000) {
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

        if (addToEpoll(*this, newClient.getFd(), EPOLLIN) == 0)
            _clients.push_back(newClient);
        else {
			std::cerr << getTimeStamp(newClient.getFd()) << RED << "Failed to add client to epoll" << RESET << std::endl;
            close(newClient.getFd());
        }
    }
}

void Webserv::handleClientActivity(int clientFd) {
    Client* client = getClientByFd(clientFd);
    if (!client) {
        std::cerr << getTimeStamp(clientFd) << RED << "Client not found" << RESET << std::endl;
        removeFromEpoll(*this, clientFd);
        close(clientFd);
        return;
    }
    client->recieveData();
}

void Webserv::handleClientDisconnect(int fd) {
    removeFromEpoll(*this, fd);

    for (size_t i = 0; i < _clients.size(); i++) {
        if (_clients[i].getFd() == fd) {
            std::cout << getTimeStamp(fd) << GREEN << "Cleaned up client connection" << RESET << std::endl;
            
            close(_clients[i].getFd());
            _clients.erase(_clients.begin() + i);
            return;
        }
    }
	if (_cgis.find(fd) != _cgis.end()) {
		std::cout << getTimeStamp(fd) << GREEN << "Cleaned up cgi-handler connection" << RESET << std::endl;
		_cgis.erase(fd);
		return;

	}
    std::cerr << getTimeStamp(fd) << RED << "Disconnect on unknown fd" << RESET << std::endl;
}

// bool Webserv::checkEventMaskErrors(uint32_t &eventMask, int &fd) {
// 	if (eventMask & EPOLLHUP) {//TODO: use EPOLLHUP
// 		handleClientDisconnect(fd);
// 		return false;
// 	}
// 	if (eventMask & EPOLLERR) {
// 		std::cerr << getTimeStamp(fd) << RED << "Socket error" << RESET << std::endl;
// 		handleErrorEvent(fd);
// 		return false;
// 	}
// 	return true;
// }

void Webserv::handleErrorEvent(int fd) {
    removeFromEpoll(*this, fd);
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

void    Webserv::cleanup() {
    for (size_t i = 0; i < _clients.size(); ++i) {
        if (_clients[i].getFd() >= 0) {
            removeFromEpoll(*this, _clients[i].getFd());
            close(_clients[i].getFd());
        }
    }

    for (size_t i = 0; i < _servers.size(); i++) {
        if (_servers[i].getFd() >= 0) {
            removeFromEpoll(*this, _servers[i].getFd());
            close(_servers[i].getFd());
        }

    }
    _servers.clear();

    if (_epollFd >= 0) {
        close(_epollFd);
        _epollFd = -1;
    }
}
