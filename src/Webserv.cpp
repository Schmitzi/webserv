#include "../include/Webserv.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/CGIHandler.hpp"
#include "../include/Helper.hpp"
#include "../include/EpollHelper.hpp"
#include "../include/ClientHelper.hpp"
#include "../include/Response.hpp"

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

std::string& Webserv::getSendBuf(int fd) {
	return _sendBuf[fd];
}

std::map<int, std::string>& Webserv::getSendBuf() {
	return _sendBuf;
}

std::map<int, CGIHandler*>& Webserv::getCgis() {
	return _cgis;
}

int Webserv::getEpollFd() {
	return _epollFd;
}

CGIHandler* Webserv::getCgiHandler(int fd) {
	std::map<int, CGIHandler*>::const_iterator it = _cgis.find(fd);
	if (it != _cgis.end())
		return it->second;
	return NULL;
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

bool Webserv::checkEventMaskErrors(uint32_t &eventMask, int fd) {
	if (eventMask & EPOLLHUP) {
		if (isCgiPipeFd(*this, fd))
			return true;
		handleClientDisconnect(fd);
		return false;
	}
	
	if (eventMask & EPOLLERR) {
		std::cerr << getTimeStamp(fd) << RED << "Error: socket" << RESET << std::endl;
		if (isCgiPipeFd(*this, fd)) {
			std::cerr << getTimeStamp(fd) << RED << "Error: CGI pipe" << RESET << std::endl;
			CGIHandler* handler = getCgiHandler(fd);
			if (handler) {
				handler->cleanupResources();
				delete handler;
			}
		} else
			handleErrorEvent(fd);
		return false;
	}
	return true;
}

int Webserv::run() {
	_epollFd = epoll_create1(EPOLL_CLOEXEC);
	if (_epollFd == -1) {
		std::cerr << getTimeStamp() << RED << "Error: epoll_create1() failed" << RESET << std::endl;
		return 1;
	}
	initialize();
	while (_state == true) {
		int nfds = epoll_wait(_epollFd, _events, MAX_EVENTS, 1);
		if (nfds == -1) {
			if (errno == EINTR)
				continue;
			std::cerr << getTimeStamp() << RED << "Error: epoll_wait() failed" << RESET << std::endl;
			continue;
		}
		for (size_t i = 0; i < _clients.size(); i++)
			checkClientTimeout(time(NULL), _clients[i]);
		checkCgiTimeouts();
		for (int i = 0; i < nfds; i++) {
			int fd = _events[i].data.fd;
			uint32_t eventMask = _events[i].events;
			if (!checkEventMaskErrors(eventMask, fd))
				continue;
			Server *activeServer = getServerByFd(fd);
			if (activeServer) {
				if (eventMask & EPOLLIN)
					handleNewConnection(*activeServer);
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

bool Webserv::checkClientTimeout(time_t now, Client &client) {
	if (now - client.lastActive() > CLIENT_TIMEOUT) {
		client.statusCode() = 408;
		client.exitErr() = true;
		client.shouldClose() = true;
		Request req = Request();
		sendErrorResponse(client, req);
		return false;
	}
	return true;
}

void Webserv::handleErrorEvent(int fd) {
	removeFromEpoll(*this, fd);
	for (size_t i = 0; i < _clients.size(); i++) {
		if (_clients[i].getFd() == fd) {
			std::cerr << getTimeStamp(fd) << RED << "Removing client due to error" << RESET << std::endl;
			close(_clients[i].getFd());
			_clients.erase(_clients.begin() + i);
			return;
		}
	}
	std::cerr << getTimeStamp(fd) << RED << "Error on unknown fd" << RESET << std::endl;
}

void Webserv::handleClientDisconnect(int fd) {    
	std::vector<int> cgiPipesToCleanup;
	for (std::map<int, CGIHandler*>::iterator it = _cgis.begin(); it != _cgis.end(); ++it) {
		CGIHandler* handler = it->second;
		if (handler && handler->getClient() && handler->getClient()->getFd() == fd)
			cgiPipesToCleanup.push_back(it->first);
	}
	
	for (size_t i = 0; i < cgiPipesToCleanup.size(); ++i) {
		int cgiFd = cgiPipesToCleanup[i];
		CGIHandler* handler = getCgiHandler(cgiFd);
		if (handler) {
			handler->cleanupResources();
			delete handler;
		}
		unregisterCgiPipe(*this, cgiFd);
	}
	removeFromEpoll(*this, fd);

	for (size_t i = 0; i < _clients.size(); i++) {
		if (_clients[i].getFd() == fd) {
			if (_clients[i].connClose() == true)
				std::cout << getTimeStamp(fd) << YELLOW << "Cleaned up and disconnected client (\"Connection: close\" in Request)" << RESET << std::endl;
			else
				std::cout << getTimeStamp(fd) << YELLOW << "Cleaned up and disconnected client" << RESET << std::endl;
			close(_clients[i].getFd());
			_clients.erase(_clients.begin() + i);
			return;
		}
	}
	std::cerr << getTimeStamp(fd) << RED << "Disconnect on unknown fd" << RESET << std::endl;
}

void Webserv::handleNewConnection(Server &server) {
	if (_clients.size() >= 1000) {
		std::cerr << getTimeStamp() << RED << "Error: 507 (insufficient storage) - can't accept any more clients for now" << RESET << std::endl;
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
	if (isCgiPipeFd(*this, clientFd)) {
		CGIHandler* handler = getCgiHandler(clientFd);
		if (handler) {
			int result = handler->processScriptOutput();
			if (result == 1) {
				handler->cleanupResources();
				unregisterCgiPipe(*this, clientFd);
				delete handler;
				
			} else if (result == 0) {
				std::cout << getTimeStamp(clientFd) << BLUE << "CGI still processing" << RESET << std::endl;
			} else {
				std::cerr << getTimeStamp(clientFd) << RED << "Error: CGI processing failed" << RESET << std::endl;
				handler->cleanupResources();
				unregisterCgiPipe(*this, clientFd);
				delete handler;
			}
		}
		return;
	}
	Client *client = getClientByFd(clientFd);
	if (!client) {
		std::cerr << getTimeStamp(clientFd) << RED << "Error: Client not found" << RESET << std::endl;
		removeFromEpoll(*this, clientFd);
		close(clientFd);
		return;
	}
	if (client->state() == UNTRACKED)
		client->state() = RECEIVING;
	if (client->state() < DONE)
		client->receiveData();
}

void Webserv::handleEpollOut(int fd) {
	Client *c = getClientByFd(fd);
	if (!c) {
		std::cerr << getTimeStamp(fd) << RED << "Error: Client not found" << RESET << std::endl;
		return;
	}
	const std::string& toSend = getSendBuf(fd);
	if (toSend.empty()) {
		std::cerr << getTimeStamp(fd) << RED << "Error: _sendBuf is empty" << RESET << std::endl;
		return;
	}

	size_t& offset = c->getOffset();
	const char* data = toSend.data() + offset;
	size_t remaining = toSend.size() - offset;

	ssize_t s = send(fd, data, remaining, 0);
	if (s < 0) {
		std::cerr << getTimeStamp(fd) << RED << "Error: send() failed" << RESET << std::endl;
		clearSendBuf(*this, fd);
		c->statusCode() = 500;
	}

	offset += static_cast<size_t>(s);
	if (offset >= toSend.size()) {
		offset = 0;
		clearSendBuf(*this, fd);
		if (c->statusCode() < 400)
			std::cout << c->output() << std::flush;
		else
			std::cerr << c->output() << std::flush;
		c->output().clear();
		if (c->shouldClose() == true)
			c->exitErr() = true;
		else {
			c->state() = UNTRACKED;
			c->exitErr() = false;
			setEpollEvents(*this, fd, EPOLLIN);
		}
	}
	if (c->exitErr() == true)
		handleClientDisconnect(fd);
}

void Webserv::checkCgiTimeouts() {
	time_t now = time(NULL);
	std::vector<int> expiredCgis;
	
	for (std::map<int, CGIHandler*>::iterator it = _cgis.begin(); it != _cgis.end(); ++it) {
		CGIHandler* handler = it->second;
		if (handler && handler->isTimedOut(now)) {
			std::cerr << getTimeStamp(it->first) << RED << "CGI timeout detected, killing process" << RESET << std::endl;
			expiredCgis.push_back(it->first);
		}
	}
	
	for (size_t i = 0; i < expiredCgis.size(); ++i) {
		int cgiFd = expiredCgis[i];
		CGIHandler* handler = getCgiHandler(cgiFd);
		if (handler) {
			handler->killProcess();
			handler->getClient()->statusCode() = 504;
			sendErrorResponse(*(handler->getClient()), handler->getRequest());
			handler->cleanupResources();
			delete handler;
			unregisterCgiPipe(*this, cgiFd);
		}
	}
}

void    Webserv::cleanup() {
	for (std::map<int, CGIHandler*>::iterator it = _cgis.begin(); it != _cgis.end(); ++it) {
		if (it->second) {
			it->second->cleanupResources();
			delete it->second;
		}
	}
	_cgis.clear();
	
	for (size_t i = 0; i < _clients.size(); ++i) {
		if (_clients[i].getFd() >= 0) {
			if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, _clients[i].getFd(), NULL) == -1) {
				if (errno != EBADF && errno != ENOENT)
					std::cerr << getTimeStamp(_clients[i].getFd()) << RED << "Warning: epoll_ctl DEL failed" << RESET << std::endl;
			}
			std::cout << getTimeStamp(_clients[i].getFd()) << "Client disconnected" << std::endl;
			close(_clients[i].getFd());
		}
	}
	_clients.clear();

	for (size_t i = 0; i < _servers.size(); i++) {
		if (_servers[i].getFd() >= 0) {
			if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, _servers[i].getFd(), NULL) == -1) {
				if (errno != EBADF && errno != ENOENT)
					std::cerr << getTimeStamp(_servers[i].getFd()) << RED << "Warning: epoll_ctl DEL failed" << RESET << std::endl;
			}
			std::cout << getTimeStamp(_servers[i].getFd()) << "Server disconnected" << std::endl;
			close(_servers[i].getFd());
		}
	}
	_servers.clear();

	if (_epollFd >= 0) {
		close(_epollFd);
		_epollFd = -1;
	}
}
