#include "../include/Webserv.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/Config.hpp"
#include "../include/ConfigParser.hpp"
#include "../include/Helper.hpp"

// Othodox Cannonical Form

Webserv::Webserv() {  
    _server = new Server();
    _confParser = ConfigParser();
	_config = Config(_confParser);//take first one by default, or choose a different one with: "Config(*_allConfigs, <nbr>)"
    // _config->printConfig();
	_server->setWebserv(this);
}

Webserv::Webserv(std::string const &config) {
	_server = new Server();
	_confParser = ConfigParser(config);
	_config = Config(_confParser);
	_server->setWebserv(this);
}

Webserv::Webserv(Webserv const &other) {
	std::cout << "Webserv copy constructed" << std::endl;
	*this = other;
}

Webserv &Webserv::operator=(Webserv const &other) {
    if (this != &other) {
		_server = other._server;
		_confParser = other._confParser;
		_config = other._config;
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

    if (_server && _server->getFd() >= 0) {
        close(_server->getFd());
    }
    
    if (_server) {
        delete _server;
        _server = NULL;
    }
    _pfds.clear();
}

Server &Webserv::getServer() {
    return *_server;
}

std::vector<struct pollfd> &Webserv::getPfds() {
    return _pfds;
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
	_config = Config(_confParser);
    return true;
}

// Add a file descriptor to the poll array
int Webserv::addToPoll(int fd, short events) {  
    // Set the socket to non-blocking mode //TODO: already set it to nonblocking in server-openSocket
    // int flags = fcntl(fd, F_GETFL, 0);
    // if (flags == -1) {
    //     ft_error("fcntl F_GETFL failed");
    //     return 1;
    // }
    
    // if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    //     ft_error("fcntl F_SETFL O_NONBLOCK failed");
    //     return 1;
    // }
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

Config Webserv::getConfig() const {
	return _config;
}

int Webserv::run() {
    // Initialize server
    if (_server->openSocket() || _server->setOptional() || _server->setServerAddr() || 
        _server->ft_bind() || _server->ft_listen()) {
        return 1;
    }
	
	int port = _config.getPort();
	if (port == -1)
		std::cerr << "No port found in config..\n";
	else {
		std::string p = tostring(port);
    	printMsg("Server is listening on port", GREEN, p);
	}

    // Initialize poll array with server socket
    addToPoll(_server->getFd(), POLLIN);

    while (1) {
        // Wait for activity on any socket
        
        if (poll(&_pfds[0], _pfds.size(), -1) < 0) {
            ft_error("poll() failed");
            continue;
        }
        
        // Check if server socket has activity (new connection)
        if (_pfds[0].revents & POLLIN) {
            // Accept the new connection
            Client* newClient = new Client(*this);
            
            if (newClient->acceptConnection() == 0) {
                // Display connection info
                newClient->displayConnection();
                // Add to poll array
                if (addToPoll(newClient->getFd(), POLLIN) == 0) {
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
        for (size_t i = 1; i < _pfds.size(); i++) {
            if (_pfds[i].revents & POLLIN) {
                // Find the corresponding client
                Client* client = NULL;
                for (size_t j = 0; j < _clients.size(); j++) {
                    if (_clients[j]->getFd() == _pfds[i].fd) {
                        client = _clients[j];
                        break;
                    }
                }
                
                if (client) {
                    // Receive data from client
                    if (client->recieveData() != 0) {
                        // Client disconnected or error, remove from poll
                        close(_pfds[i].fd);
                        
                        // Remove client from vector
                        for (size_t j = 0; j < _clients.size(); j++) {
                            if (_clients[j]->getFd() == _pfds[i].fd) {
                                delete _clients[j];
                                _clients.erase(_clients.begin() + j);
                                break;
                            }
                        }
                        removeFromPoll(i);
                        i--;
                    }
                }
            }
        }
    }
    close(_server->getFd());
    return 0;
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
	