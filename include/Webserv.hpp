#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <sys/stat.h>
#include <sstream>
#include <ctime>
#include <poll.h>

class Server;
class Client;
class Config;

// COLOURS
#define BLUE    "\33[34m"
#define GREEN   "\33[32m"
#define RED     "\33[31m"
#define WHITE   "\33[97m"
#define RESET   "\33[0m" // No Colour

class Webserv {
    public:
        Webserv();
        Webserv(std::string const &config);
        Webserv(Webserv const &other);
        Webserv &operator=(Webserv const &other);
        ~Webserv();
        int setConfig(std::string const filepath);
        struct pollfd *getPfds();
        int    run();
        void    ft_error(std::string const msg);
        std::string getTimeStamp();
        void    printMsg(const std::string msg, char const *colour, std::string const opt);
    private:
        Client  *_client;
        Server  *_server;
        //Config  *_config;
        struct pollfd _pfds[1];       
};

#endif
