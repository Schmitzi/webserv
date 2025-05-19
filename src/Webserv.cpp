#include "../include/Webserv.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/ConfigParser.hpp"
#include "../include/Helper.hpp"

// Othodox Cannonical Form

Webserv::Webserv() { 
    _confParser = ConfigParser();
	_configs = _confParser.getAllConfigs();
    for (size_t i = 0; i < _configs.size(); i++) {
        _servers.push_back(new Server(_confParser, i));
        _servers[i]->setWebserv(this);
    }
}

Webserv::Webserv(std::string const &config) {
	_confParser = ConfigParser(config);
	_configs = _confParser.getAllConfigs();
	for (size_t i = 0; i < _confParser.getAllConfigs().size(); i++) {
		_servers.push_back(new Server(_confParser, i));
		_servers[i]->setWebserv(this);
	}
}

Webserv::Webserv(Webserv const &other) {
    *this = other;
    *this = other;
}

Webserv &Webserv::operator=(Webserv const &other) {
    if (this != &other) {
		for (size_t i = 0; i < other._servers.size(); i++)
			_servers.push_back(other._servers[i]);
		for (size_t i = 0; i < other._clients.size(); i++)
			_clients.push_back(other._clients[i]);
		for (size_t i = 0; i < other._pfds.size(); i++)
			_pfds.push_back(other._pfds[i]);
		_env = other._env;
		_confParser = other._confParser;
		for (size_t i = 0; i < other._configs.size(); i++)
			_configs.push_back(other._configs[i]);
	}
	return *this;
}

Webserv::~Webserv() {
    for (size_t i = 0; i < _clients.size(); ++i) {
        if (_clients[i]->getFd() >= 0) {
            close(_clients[i]->getFd());
        }
        delete _clients[i];
    }
    _clients.clear();

    for (size_t i = 0; i < _servers.size(); i++)
    if (!_servers.empty() && _servers[i]->getFd() >= 0) {
        close(_servers[i]->getFd());
        delete _servers[i];
        _servers[i] = NULL;
    }
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
   // Initialize server
   for (size_t i = 0; i < _servers.size(); i++) {
    std::cout << BLUE << getTimeStamp() << "Initializing server " << i + 1 << " with port " << RESET << _servers[i]->getConfigClass().getPort() << std::endl;
    
    if (_servers[i]->openSocket() || _servers[i]->setOptional() || 
        _servers[i]->setServerAddr() || _servers[i]->ft_bind() || _servers[i]->ft_listen()) {
        std::cerr << RED << getTimeStamp() << "Failed to initialize server: " << RESET << i + 1 << std::endl;
        continue; // Skip this server but try to initialize others
    }
    
    // Add server socket to poll array
    addToPoll(_servers[i]->getFd(), POLLIN);
    
    std::cout << GREEN << getTimeStamp() << 
        "Server " << i + 1 << " is listening on port " << RESET << 
        _servers[i]->getConfigClass().getPort() << "\n";
    }

    while (1) {
        // Poll the master array containing all server and client fds
        if (poll(&_pfds[0], _pfds.size(), -1) < 0) {
            ft_error("poll() failed");
            continue;
        }
        
        // Process all events
        for (size_t i = 0; i < _pfds.size(); i++) {
            if (!(_pfds[i].revents & POLLIN)) {
                continue;
            }
            
            // Check if this is a server listening socket
            Server* activeServer = NULL;
            for (size_t j = 0; j < _servers.size(); j++) {
                if (_servers[j]->getFd() == _pfds[i].fd) {
                    activeServer = _servers[j];
                    break;
                }
            }
            
            if (activeServer) {
                // New connection on a server socket
                handleNewConnection(*activeServer);
            } else {
                // Activity on a client socket
                handleClientActivity(i);
            }
        }
    }

    // Clean up
    for (size_t i = 0; i < _servers.size(); i++) {
        close(_servers[i]->getFd());
    }
    return 0;
}

// Add a file descriptor to the poll array
int Webserv::addToPoll(int fd, short events) {  
    // Add to poll array
    struct pollfd temp;
    temp.fd = fd;
    temp.events = events;
    temp.revents = 0;
    _pfds.push_back(temp);
    
    return 0;
}

// Remove a file descriptor from the poll array by index
void Webserv::removeFromPoll(size_t index) {
    if (index >= _pfds.size()) {
        printMsg("Invalid poll index", RED, "");
        return;
    }
    _pfds.erase(_pfds.begin() + index);
}

void Webserv::handleNewConnection(Server &server) {
    // Create a new client associated with this Webserv instance
    Client* newClient = new Client(server);
    
    // Set the specific server that accepted this connection
    // newClient->setServer(server);
    
    // Now let the client accept the actual connection from the server socket
    if (newClient->acceptConnection(server.getFd()) == 0) {
        // Connection accepted successfully
        newClient->displayConnection();
        
        // Add client to poll array and client list
        if (addToPoll(newClient->getFd(), POLLIN) == 0) {
            _clients.push_back(newClient);
        } else {
            printMsg("Failed to add client to poll", RED, "");
            delete newClient;
        }
    } else {
        // Failed to accept connection
        delete newClient;
    }
}

void Webserv::handleClientActivity(size_t pollIndex) {
    Client* client = NULL;
    for (size_t j = 0; j < _clients.size(); j++) {
        if (_clients[j]->getFd() == _pfds[pollIndex].fd) {
            client = _clients[j];
            break;
        }
    }
    
    if (client && client->recieveData() != 0) {
        // Client disconnected or error
        close(_pfds[pollIndex].fd);
        
        for (size_t j = 0; j < _clients.size(); j++) {
            if (_clients[j]->getFd() == _pfds[pollIndex].fd) {
                delete _clients[j];
                _clients.erase(_clients.begin() + j);
                break;
            }
        }
        removeFromPoll(pollIndex);
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
