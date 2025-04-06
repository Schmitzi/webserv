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
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/Config.hpp"

class Webserv {
    public:
        Webserv();
        Webserv(std::string const &config);
        Webserv(Webserv const &other);
        Webserv &operator=(Webserv const &other);
        ~Webserv();
        int setConfig(std::string const filepath);
        int    run();
        void    ft_error(std::string const msg);
        std::string getTimeStamp();
    private:
        Client  _client;
        Server  _server;
        Config  _config;

        int openSocket();
        int setOptional();
        int setServerAddr();
        int ft_bind();
        int ft_listen();
};

#endif
