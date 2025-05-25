#include "../include/Webserv.hpp"


// Othodox Cannonical Form

Webserv::Webserv() : _epollFd(-1) { 
    _confParser = ConfigParser();
	_configs = _confParser.getAllConfigs();
    for (size_t i = 0; i < _configs.size(); i++) {
        _servers.push_back(new Server(_confParser, i));
        _servers[i]->setWebserv(this);
    }
}

Webserv::Webserv(std::string const &config) : _epollFd(-1) {
	_confParser = ConfigParser(config);
	_configs = _confParser.getAllConfigs();
	for (size_t i = 0; i < _confParser.getAllConfigs().size(); i++) {
		_servers.push_back(new Server(_confParser, i));
		_servers[i]->setWebserv(this);
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

Server &Webserv::getServer(int i) {
    return *_servers[i];
}

void Webserv::setEnvironment(char **envp) {
    _env = envp;
}

char **Webserv::getEnvironment() const {
    return _env;
}

int Webserv::setConfig(std::string const filepath) {
    std::cout << GREEN << getTimeStamp() << "Config found at " << RESET << filepath << "\n";
	_confParser = ConfigParser(filepath);
	_configs = _confParser.getAllConfigs();
    return true;
}

Config &Webserv::getDefaultConfig() {
	for (size_t i = 0; i < _servers.size(); i++) {
		serverLevel conf = _servers[i]->getConfigClass().getConfig();
		for (size_t j = 0; j < conf.port.size(); j++) {
			if (conf.port[j].second == true)
				return _servers[i]->getConfigClass();
		}
	}
	return _servers[0]->getConfigClass();
}

int Webserv::run() {
    // Create epoll
    _epollFd = epoll_create1(EPOLL_CLOEXEC);
    if (_epollFd == -1) {
        ft_error("epoll_create1() failed");
        return 1;
    }

   // Initialize server
   for (size_t i = 0; i < _servers.size(); i++) {
    std::cout << BLUE << getTimeStamp() << "Initializing server " << i + 1 << " with port " << RESET << _servers[i]->getConfigClass().getPort() << std::endl;
    
    if (_servers[i]->openSocket() || _servers[i]->setOptional() || 
        _servers[i]->setServerAddr() || _servers[i]->ft_bind() || _servers[i]->ft_listen()) {
        std::cerr << RED << getTimeStamp() << "Failed to initialize server: " << RESET << i + 1 << std::endl;
        continue; // Skip this server but try to initialize others
    }
    
    // Add server socket to poll array
    if (addToEpoll(_servers[i]->getFd(), EPOLLIN) != 0) {
        std::cerr << RED << getTimeStamp() << "Failed to add server to epoll: " << RESET << i + 1 << std::endl;
        continue;
    }
    
    std::cout << GREEN << getTimeStamp() << 
        "Server " << i + 1 << " is listening on port " << RESET << 
        _servers[i]->getConfigClass().getPort() << "\n";
    }

    

    while (1) {
        // Wait for events
        int nfds = epoll_wait(_epollFd, _events, MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR) {
                continue; // Interrupted by signal, continue
            }
            ft_error("epoll_wait() failed");
            continue;
        }
        
        // Process all events
        for (int i = 0; i < nfds; i++) {
            int fd = _events[i].data.fd;
            uint32_t eventMask = _events[i].events;
            
            // Check for errors
            if (eventMask & (EPOLLERR | EPOLLHUP)) {
                std::cerr << RED << getTimeStamp() << "Error on fd " << fd << RESET << std::endl;
                handleErrorEvent(fd);
                continue;
            }
            
            // Check if this is a server listening socket
            Server* activeServer = findServerByFd(fd);
            
            if (activeServer) {
                // New connection on a server socket
                if (eventMask & EPOLLIN) {
                    handleNewConnection(*activeServer);
                }
            } else {
                // Activity on a client socket
                if (eventMask & EPOLLIN) {
                    handleClientActivity(fd);
                }
            }
        }
    }

    return 0;
}

Server* Webserv::findServerByFd(int fd) {
    for (size_t i = 0; i < _servers.size(); i++) {
        if (_servers[i]->getFd() == fd) {
            return _servers[i];
        }
    }
    return NULL;
}

Client* Webserv::findClientByFd(int fd) {
    for (size_t i = 0; i < _clients.size(); i++) {
        if (_clients[i]->getFd() == fd) {
            return _clients[i];
        }
    }
    return NULL;
}

void Webserv::handleErrorEvent(int fd) {
    // Find and remove client if it exists
    for (size_t i = 0; i < _clients.size(); i++) {
        if (_clients[i]->getFd() == fd) {
            std::cout << RED << getTimeStamp() << "Removing client due to error: " << fd << RESET << std::endl;
            
            // Remove from epoll first
            removeFromEpoll(fd);
            
            // Close and delete client
            close(_clients[i]->getFd());
            delete _clients[i];
            _clients.erase(_clients.begin() + i);
            return;
        }
    }
}

// Add a file descriptor to the poll array
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

// Remove a file descriptor from the poll array by index
void Webserv::removeFromEpoll(int fd) {
    if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        // Don't treat this as a fatal error since the fd might already be closed
        std::cerr << "Warning: epoll_ctl DEL failed for fd " << fd << ": " << strerror(errno) << std::endl;
    }
}

void Webserv::handleNewConnection(Server &server) {
    // Create a new client associated with this Webserv instance
    Client* newClient = new Client(server);
    
    // Now let the client accept the actual connection from the server socket
    if (newClient->acceptConnection(server.getFd()) == 0) {
        // Connection accepted successfully
        newClient->displayConnection();
        
        // Add client to epoll and client list
        if (addToEpoll(newClient->getFd(), EPOLLIN) == 0) {
            _clients.push_back(newClient);
        } else {
            printMsg("Failed to add client to epoll", RED, "");
            close(newClient->getFd());
            delete newClient;
        }
    } else {
        // Failed to accept connection
        delete newClient;
    }
}

void Webserv::handleClientActivity(int clientFd) {
    Client* client = findClientByFd(clientFd);
    
    if (!client) {
        std::cerr << RED << getTimeStamp() << "Client not found for fd: " << clientFd << RESET << std::endl;
        removeFromEpoll(clientFd);
        close(clientFd);
        return;
    }
    
    if (client->recieveData() != 0) {
        // Client disconnected or error
        std::cout << RED << getTimeStamp() << "Client disconnected: " << clientFd << RESET << "\n";
        
        // Remove from epoll first
        removeFromEpoll(clientFd);
        
        // Close and remove client
        close(clientFd);
        
        for (size_t i = 0; i < _clients.size(); i++) {
            if (_clients[i]->getFd() == clientFd) {
                delete _clients[i];
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
    // Close all client connections
    for (size_t i = 0; i < _clients.size(); ++i) {
        if (_clients[i]->getFd() >= 0) {
            close(_clients[i]->getFd());
        }
        delete _clients[i];
    }
    _clients.clear();

    // Close all server sockets
    for (size_t i = 0; i < _servers.size(); i++) {
        if (_servers[i] && _servers[i]->getFd() >= 0) {
            close(_servers[i]->getFd());
            delete _servers[i];
            _servers[i] = NULL;
        }
    }
    _servers.clear();

    // Close epoll file descriptor
    if (_epollFd >= 0) {
        close(_epollFd);
        _epollFd = -1;
    }
}
