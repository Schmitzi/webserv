#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <netinet/in.h>
#include <iostream>
#include <cstring>
#include <map>

#define BLUE    "\33[34m"
#define GREEN   "\33[32m"
#define RED     "\33[31m"
#define WHITE   "\33[97m"
#define RESET   "\33[0m" // No Colour

class Webserv;
class Server;

class Client {
    public:
        Client();
        Client(Webserv &other);
        ~Client();
        struct sockaddr_in      &getAddr();
        socklen_t               &getAddrLen();
        int                     &getFd();
        unsigned char           &getIP();
        char                    &getBuffer();
        void setWebserv(Webserv *webserv);
        void setServer(Server *server);
        int acceptConnection();
        void displayConnection();
        int recieveData();
    private:
        struct sockaddr_in  _addr;
        socklen_t           _addrLen;
        int                 _fd;
        unsigned char       *_ip;
        char				_buffer[1024];

        Webserv             *_webserv;
        Server              *_server;
};

#endif