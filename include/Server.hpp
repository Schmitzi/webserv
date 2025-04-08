#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

#define BLUE    "\33[34m"
#define GREEN   "\33[32m"
#define RED     "\33[31m"
#define WHITE   "\33[97m"
#define RESET   "\33[0m" // No Colour

class Webserv;

class Server {
    public:
        Server();
        ~Server();
        
        Webserv             &getWebServ();
        struct sockaddr_in  &getAddr();
        int                 &getFd();
        void                setFd(int const fd);
        void                setWebserv(Webserv* webserv); // Add a setter for the webserv pointer
        int                 openSocket();
        int                 setOptional();
        int                 setServerAddr();
        int                 ft_bind();
        int                 ft_listen();
    private:
        int                 _fd;
        struct sockaddr_in  _addr;
        
        Webserv             *_webserv;
};

#endif