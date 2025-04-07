#include "../include/Webserv.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/Config.hpp"


// Othodox Cannonical Form

Webserv::Webserv() {
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

int Webserv::run() {
    if (_server->openSocket() || _server->setOptional() || _server->setServerAddr() || \
         _server->ft_bind() || _server->ft_listen()) {
        return 1;
    }

    printMsg("Server is listening on port", GREEN, "8080"); //_address.sin_port

    _pfds[0].fd = 0;
    _pfds[0].events = POLLIN;

    while (1) { // Accept connections and handle them in a loop
        // Accept connections
        if (_client->acceptConnection() == 1) {
            continue;
        }
        
        // Display client connection info
        _client->displayConnection();
        
        // Handle communication with this client
        while (1) {
            if (_client->recieveData() == 1) {
                break;
            }
        }
        close(_client->_fd);
    }
    close(_server->_fd);
    return 0;
}

void    Webserv::ft_error(std::string const msg) {
    //std::cerr << RED << getTimeStamp() << "Error: " << msg << ": " << strerror(errno) << RESET << "\n";
    
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
    // Use poll with a timeout of 0 to get current time
    struct pollfd pfd;
    pfd.fd = -1;  // Invalid fd
    pfd.events = POLLIN;
    
    // Poll returns immediately due to invalid fd
    poll(&pfd, 1, 1);  // Wait for 1ms max    NEEDS TO BE CHANGED
    
    // Get current time manually
    time_t now = time(NULL);  // time() is a standard function
    struct tm* tm_info = localtime(&now);
    
    std::ostringstream oss;
    oss << "[" 
        << (tm_info->tm_year + 1900) << "-"
        << (tm_info->tm_mon + 1) << "-"
        << tm_info->tm_mday << " "
        << tm_info->tm_hour << ":"
        << tm_info->tm_min << ":"
        << tm_info->tm_sec << "] ";
    
    return oss.str();
}

struct pollfd *Webserv::getPfds() {
    return _pfds;
}