#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <iostream>
#include <string>
#include <netinet/in.h>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include "../include/config.hpp"

class Webserv {
    public:
        Webserv();
        Webserv(const std::string& config);
        Webserv& operator=(Webserv W);
        ~Webserv();

        int run();
    private:
        Config _config;
        int         _server_fd;
        std::string _config;
        sockaddr_in _address;
        void        setup();
};

#endif