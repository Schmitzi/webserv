#include "../include/Webserv.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/Config.hpp"


// Othodox Cannonical Form

Webserv::Webserv() : _nfds(0) {
    //std::cout << "Webserv constructed" << std::endl;
    // Create objects first
    _server = new Server();
    _client = new Client();
    //_config = new Config(Config);

    _server->setWebserv(this);
    _client->setWebserv(this);
    _client->setServer(_server);
}

Webserv::Webserv(std::string const &config) {
   std::cout << "Webserv copy constructed" << std::endl;
   (void) config;
   //_config = config;
}

Webserv::Webserv(Webserv const &other) {
    (void) other;
    //_config = other._config;
}

Webserv &Webserv::operator=(Webserv const &other) {
    (void) other;
    // _config = other._config;
	return *this;
}

Webserv::~Webserv() {
    //std::cout << "Webserv deconstructed" << std::endl;
}

int Webserv::setConfig(std::string const filepath) {
    std::cout << getTimeStamp() << "Config found at " << filepath << "\n";
    return true;
}

// Add a file descriptor to the poll array
int Webserv::addToPoll(int fd, short events) {
    if (_nfds >= MAX_CLIENTS + 1) {
        printMsg("Poll array is full", RED, "");
        return 1; // Error - too many file descriptors
    }
    
    // Set the socket to non-blocking mode
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        ft_error("fcntl F_GETFL failed");
        return 1;
    }
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        ft_error("fcntl F_SETFL O_NONBLOCK failed");
        return 1;
    }
    
    // Add to poll array
    _pfds[_nfds].fd = fd;
    _pfds[_nfds].events = events;
    _nfds++;
    
    return 0;
}

// Remove a file descriptor from the poll array by index
void Webserv::removeFromPoll(int index) {
    if (index < 0 || index >= _nfds) {
        printMsg("Invalid poll index", RED, "");
        return;
    }
    
    // Move all higher elements down by one
    for (int i = index; i < _nfds - 1; i++) {
        _pfds[i] = _pfds[i + 1];
    }
    
    // Decrement count
    _nfds--;
}

int Webserv::run() {
    // Initialize server
    if (_server->openSocket() || _server->setOptional() || _server->setServerAddr() || 
        _server->ft_bind() || _server->ft_listen()) {
        return 1;
    }

    printMsg("Server is listening on port", GREEN, "8080");

    // Initialize poll array with server socket
    _nfds = 0; // Start with 0 and add the server
    addToPoll(_server->_fd, POLLIN);

    while (1) {
        // Wait for activity on any socket
        
        if ( poll(_pfds, _nfds, -1) < 0) {
            ft_error("poll() failed");
            continue;
        }
        
        // Check if server socket has activity (new connection)
        if (_pfds[0].revents & POLLIN) {
            // Accept the new connection
            Client* newClient = new Client(*_client);
            
            if (newClient->acceptConnection() == 0) {
                // Add to poll array
                if (addToPoll(newClient->_fd, POLLIN) == 0) {
                    // Store client for later use
                    _clients.push_back(newClient);
                    
                    // Display connection info
                    newClient->displayConnection();
                } else {
                    printMsg("Failed to add client to poll", RED, "");
                    delete newClient;
                }
            } else {
                delete newClient;
            }
        }
        
        // Check client sockets for activity
        for (int i = 1; i < _nfds; i++) {
            if (_pfds[i].revents & POLLIN) {
                // Find the corresponding client
                Client* client = NULL;
                for (size_t j = 0; j < _clients.size(); j++) {
                    if (_clients[j]->_fd == _pfds[i].fd) {
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
                            if (_clients[j]->_fd == _pfds[i].fd) {
                                delete _clients[j];
                                _clients.erase(_clients.begin() + j);
                                break;
                            }
                        }
                        
                        // Remove from poll array
                        removeFromPoll(i);
                        i--; // Adjust index since we removed an element
                    }
                }
            }
        }
    }
    close(_server->_fd);
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

struct pollfd *Webserv::getPfds() {
    return _pfds;
}