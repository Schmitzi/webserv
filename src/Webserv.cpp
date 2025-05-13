#include "../include/Webserv.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/ConfigParser.hpp"
#include "../include/Helper.hpp"

// Othodox Cannonical Form

Webserv::Webserv() { 
    _confParser = ConfigParser();
	_configs = _confParser.getAllConfigs();
    for (size_t i = 0; i < _confParser.getAllConfigs().size(); i++) {
        _servers.push_back(new Server());
        _servers[i]->setWebserv(this);
        Config* temp = new Config(_confParser, i);
        _servers[i]->setConfig(*temp);
    }
}

Webserv::Webserv(std::string const &config) {
	_servers[0] = new Server();
	_confParser = ConfigParser(config);
	_configs = _confParser.getAllConfigs();
	_servers[0]->setWebserv(this);
}

Webserv::Webserv(Webserv const &other) {
	*this = other;
}

Webserv &Webserv::operator=(Webserv const &other) {
    if (this != &other) {
		for (size_t i = 0; i < other._servers.size(); i++)
			_servers.push_back(other._servers[i]);
		for (size_t i = 0; i < other._clients.size(); i++)
			_clients.push_back(other._clients[i]);
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

Server &Webserv::getServer() {
    return *_servers[0];//TODO: change this to return the right server
}

void Webserv::setEnvironment(char **envp) {
    _env = envp;
}

char **Webserv::getEnvironment() const {
    return _env;
}

int Webserv::setConfig(std::string const filepath) {
    std::cout << getTimeStamp() << "Config found at " << filepath << "\n";
	_confParser = ConfigParser(filepath);
	_configs = _confParser.getAllConfigs();
    return true;
}

std::vector<Server *> &Webserv::getServers() {
	return _servers;
}

Config Webserv::getDefaultConfig() {
	for (size_t i = 0; i < _servers.size(); i++) {
		serverLevel conf = _servers[i]->getConfigClass().getConfig();
		for (size_t j = 0; j < conf.port.size(); j++) {
			if (conf.port[j].second == true)
				return _servers[i]->getConfigClass();
		}

	}
	return _servers[0]->getConfigClass();
}

Config &Webserv::getSpecificConfig(std::string& serverName, int port) {//TODO: USE IT!
	for (size_t i = 0; i < _servers.size(); i++) {
		serverLevel conf = _servers[i]->getConfigClass().getConfig();
		for (size_t j = 0; j < conf.port.size(); j++) {
			if (conf.servName[0] == serverName && conf.port[j].first.first == port)
				return _servers[i]->getConfigClass();
		}
	}
	return _servers[0]->getConfigClass();
}

int Webserv::run() {
    // Initialize server
    if (getServer().openSocket() || getServer().setOptional() || getServer().setServerAddr() || 
        getServer().ft_bind() || getServer().ft_listen()) {
        return 1;
    }
	
	int port = getDefaultConfig().getPort();
	if (port == -1)
		std::cerr << "No port found in config..\n";
	else {
		std::string p = tostring(port);
    	printMsg("Server is listening on port", GREEN, p);
	}

    // Initialize poll array with server socket
    getServer().addToPoll(getServer().getFd(), POLLIN);

    while (1) {
        // Wait for activity on any socket
        if (poll(&getServer().getPfds()[0], getServer().getPfds().size(), -1) < 0) {
            ft_error("poll() failed");
            continue;
        }
        
        // Check if server socket has activity (new connection)
        if (getServer().getPfds()[0].revents & POLLIN) {
            // Accept the new connection
            Client* newClient = new Client(*this);
            
            if (newClient->acceptConnection() == 0) {
                // Display connection info
                newClient->displayConnection();
                // Add to poll array
                if (getServer().addToPoll(newClient->getFd(), POLLIN) == 0) {
                    // Store client for later use
                    _clients.push_back(newClient);
                    
                } else {
                    printMsg("Failed to add client to poll", RED, "");
                    delete newClient;
                }
            } else {
                delete newClient;
            }
        }
        
        // Check client sockets for activity
        for (size_t i = 1; i < getServer().getPfds().size(); i++) {
            if (getServer().getPfds()[i].revents & POLLIN) {
                // Find the corresponding client
                Client* client = NULL;
                for (size_t j = 0; j < _clients.size(); j++) {
                    if (_clients[j]->getFd() == getServer().getPfds()[i].fd) {
                        client = _clients[j];
                        break;
                    }
                }
                
                if (client) {
                    // Receive data from client
                    if (client->recieveData() != 0) {
                        // Client disconnected or error, remove from poll
                        close(getServer().getPfds()[i].fd);
                        
                        // Remove client from vector
                        for (size_t j = 0; j < _clients.size(); j++) {
                            if (_clients[j]->getFd() == getServer().getPfds()[i].fd) {
                                delete _clients[j];
                                _clients.erase(_clients.begin() + j);
                                break;
                            }
                        }
                        getServer().removeFromPoll(i);
                        i--;
                    }
                }
            }
        }
    }
    close(getServer().getFd());
    return 0;
}

void    Webserv::ft_error(std::string const msg) {  // TODO: Check if allowed
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

//TODO: idk, just a try
// void Webserv::initServers() {
// 	std::vector<serverLevel> confs = _confParser->getAllConfigs();
// 	for (size_t i = 0; i < confs.size(); i++) {
// 		Server server;
// 		std::vector<std::pair<std::string, int> >::iterator it = confs[i].port.begin();
// 		for (; it != confs[i].port.end(); ++it)
// 			server.addListenPort(it->first, it->second);
// 		server.setWebserv(this);
// 		if (server.openAndListenSockets() != 0)
// 			return (ft_error("Failed to initialize server"));
// 		_servers.push_back(server);
// 	}
// }
	