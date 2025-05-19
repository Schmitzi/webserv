#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include "Config.hpp"
#include "Client.hpp"
#include "Request.hpp"

#define BLUE    "\33[34m"
#define GREEN   "\33[32m"
#define RED     "\33[31m"
#define WHITE   "\33[97m"
#define RESET   "\33[0m" // No Colour

// Forward declaration
class Webserv;
class Config;
class Request;
class Client;

class Server {
    public:
        Server(ConfigParser confs, int nbr);
        ~Server();
        
        Webserv             &getWebServ();
        struct sockaddr_in  &getAddr();
        int                 &getFd();
        std::string   		getUploadDir(Client& client, Request& req);
        std::string const   &getWebRoot();
        Config              &getConfigClass();
        void                setFd(int const fd);
        void                setWebserv(Webserv* webserv);
        void                setConfig(Config config);
        int                 openSocket();
        int                 setOptional();
        int                 setServerAddr();
        int                 ft_bind();
        int                 ft_listen();
    private:
        Server();
        int                 _fd;
        struct sockaddr_in  _addr;
        std::string         _uploadDir;
        std::string         _webRoot;
        Config              _config;
        
        Webserv             *_webserv;
};

#endif