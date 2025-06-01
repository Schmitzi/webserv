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
// #include <poll.h>
#include <sys/epoll.h>
#include <iomanip>
#include <vector>
#include <signal.h>
#include <map>
// #include "../include/Config.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/ConfigParser.hpp"
#include "../include/Helper.hpp"

// Forward declarations
class Server;
class Client;
// class Config;

// COLOURS
#define BLUE    "\33[34m"
#define GREEN   "\33[32m"
#define RED     "\33[31m"
#define WHITE   "\33[97m"
#define RESET   "\33[0m" // No Colour

// Others
#define MAX_EVENTS 64

class Webserv {
    public:
        Webserv();
        Webserv(std::string const &config);
        Webserv(Webserv const &other);
        Webserv &operator=(Webserv const &other);
        ~Webserv();

        Server          &getServer(int i);
		ConfigParser    &getConfigParser();
        void            setEnvironment(char **envp);
        char            **getEnvironment() const;
        int             setConfig(std::string const filepath);
        // Polling
        //int             serverCheck();
        Server*         findServerByFd(int fd);
        Client*         findClientByFd(int fd);
        void            handleErrorEvent(int fd);
        int             addToEpoll(int fd, short events);
        void            removeFromEpoll(int fd);
        void            handleClientDisconnect(int fd);
        int             run();
        void            handleNewConnection(Server& server);
        void            handleClientActivity(int clientFd);
        void            ft_error(std::string const msg);
        std::string     getTimeStamp();
        void            printMsg(const std::string msg, char const *colour, std::string const opt);
        serverLevel     &getDefaultConfig();
        void            cleanup();

    private:
        std::vector<Server *> 		_servers;
        std::vector<Client *>   	_clients;
        char                    	**_env;
        int                         _epollFd;
        struct epoll_event          _events[MAX_EVENTS];
        ConfigParser				_confParser;
        std::vector<serverLevel>	_configs; 
        
};

#endif