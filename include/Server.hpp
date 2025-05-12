#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include "Config.hpp"

#define BLUE    "\33[34m"
#define GREEN   "\33[32m"
#define RED     "\33[31m"
#define WHITE   "\33[97m"
#define RESET   "\33[0m" // No Colour

// Forward declaration
class Webserv;
class Config;

class Server {
    public:
        Server();
        Server  &operator=(Server const &other);
        ~Server();
        
        Webserv             &getWebServ();
        struct sockaddr_in  &getAddr();
        int                 &getFd();
        std::string const   &getUploadDir();
        std::string const   &getWebRoot();
        std::vector<struct pollfd> &getPfds();
        void                addPfd(struct pollfd newPfd);
        void                removePfd(int index);
        void                setFd(int const fd);
        void                setWebserv(Webserv* webserv);
        void                setConfig(Config config);
        // Polling
        int                 addToPoll(int fd, short events);
        void                removeFromPoll(size_t index);

        int                 openSocket();
        int                 setOptional();
        int                 setServerAddr();
        int                 ft_bind();
        int                 ft_listen();
		// int					openAndListenSockets();
    private:
        int                 _fd;
        struct sockaddr_in  _addr;
        std::string         _uploadDir;
        std::string         _webRoot;
        std::vector<struct pollfd> _pfds;
        Config              _config;
        
        Webserv             *_webserv;
};

#endif