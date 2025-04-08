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
#include <iomanip>
#include <vector>
#include <fcntl.h>
#include <signal.h>

class Server;
class Client;
class Config;

// COLOURS
#define BLUE    "\33[34m"
#define GREEN   "\33[32m"
#define RED     "\33[31m"
#define WHITE   "\33[97m"
#define RESET   "\33[0m" // No Colour

#define MAX_CLIENTS 30

class Webserv {
    public:
        Webserv();
        Webserv(std::string const &config);
        Webserv(Webserv const &other);
        Webserv &operator=(Webserv const &other);
        ~Webserv();

        Server          &getServer();
        std::vector<struct pollfd> &getPfds();
        int             setConfig(std::string const filepath);
        int             run();
        void            ft_error(std::string const msg);
        std::string     getTimeStamp();
        void            printMsg(const std::string msg, char const *colour, std::string const opt);
    private:
        Server                  *_server;
        std::vector<Client *>   _clients;
        //Config                *_config;

        //struct pollfd   _pfds[MAX_CLIENTS + 1];
        std::vector<struct pollfd> _pfds;
        //int             _nfds;

        // Polling

        int             addToPoll(int fd, short events);
        void            removeFromPoll(size_t index);
};

#endif
