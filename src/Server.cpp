#include "../include/Server.hpp"
#include "../include/Webserv.hpp"

Server::Server() : _uploadDir("local/upload/"), _webRoot("local"), _webserv(NULL) {

}

Server::Server(const Server& other) : 
    _fd(other._fd),
    _addr(other._addr),
    _uploadDir(other._uploadDir),
    _webRoot(other._webRoot),
    _webserv(other._webserv)
{
}

Server& Server::operator=(const Server& other) {
    if (this != &other) {
        _uploadDir = other._uploadDir;
        _webRoot = other._webRoot;
        _webserv = other._webserv;
        _fd = other._fd;
        _addr = other._addr;
    }
    return *this;
}

Server::~Server()  {

}

Webserv &Server::getWebServ() {
    return *_webserv;
}

struct sockaddr_in  &Server::getAddr() {
    return _addr;
}

int &Server::getFd() {
    return _fd;
}

std::string const &Server::getUploadDir() {
    return _uploadDir;
}

std::string const   &Server::getWebRoot() {
    return _webRoot;
}

void Server::setWebserv(Webserv* webserv) {
    _webserv = webserv;
}

void    Server::setFd(int const fd) {
    _fd = fd;
}

int Server::openSocket() { // Create a TCP socket
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd < 0) {
        _webserv->ft_error("Socket creation error");
        return 1;
    }
    _webserv->printMsg("Server started", GREEN, "");
    return 0;
}

int Server::setOptional() { // Optional: set socket options to reuse address
    int opt = 1;
    if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        _webserv->ft_error("setsockopt() failed");
        close(_fd);
        return 1;
    }
    return 0;
}

void Server::setConfig(Config* config) {
    _config = config;
    
    // Set up server properties from config
    serverLevel serverConf = config->getConfig();
    if (!serverConf.rootServ.empty())
        _webRoot = serverConf.rootServ;
    
    // Handle upload directories from locations
    std::map<std::string, locationLevel>::iterator it = serverConf.locations.find("/upload");
    if (it != serverConf.locations.end() && !it->second.uploadDirPath.empty())
        _uploadDir = it->second.uploadDirPath;
}

Config* Server::getConfig() const {
    return _config;
}

int Server::ft_bind() { // Bind socket to address
    if (bind(_fd, (struct sockaddr *)&_addr, sizeof(_addr)) < 0) {
        _webserv->ft_error("bind() failed");
        close(_fd);
        return 1;
    }
    return 0;
}

int Server::ft_listen() { // Listen for connections
    if (listen(_fd, 3) < 0) {
        _webserv->ft_error("listen() failed");
        close(_fd);
        return 1;
    }
    return 0;
}

int Server::setServerAddr() {
    memset(&_addr, 0, sizeof(_addr));
    _addr.sin_family = AF_INET;          // IPv4 Internet Protocol
    _addr.sin_addr.s_addr = INADDR_ANY;  // Accept connections on any interface
    _addr.sin_port = htons(getWebServ().getConfig().getPort());
    return 0;
}