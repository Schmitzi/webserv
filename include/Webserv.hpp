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
#include <sys/epoll.h>
#include <iomanip>
#include <vector>
#include <signal.h>
#include <map>
#include "Server.hpp"
#include "Client.hpp"
#include "ConfigParser.hpp"
#include "Helper.hpp"

// Forward declarations
class Server;
class Client;

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
        Webserv(std::string const &config = "config/default.conf");
        Webserv(Webserv const &other);
        Webserv &operator=(Webserv const &other);
        ~Webserv();

        void            setEnvironment(char **envp);
        // Polling
        Server         findServerByFd(int fd, bool& found);
        Client         findClientByFd(int fd, bool& found);
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
        void            cleanup();

    private:
        bool                    _state;
        std::vector<Server> 		_servers;
        std::vector<Client>   	    _clients;
        char                    	**_env;
        int                         _epollFd;
        struct epoll_event          _events[MAX_EVENTS];
        ConfigParser				_confParser;
        std::vector<serverLevel>	_configs; 
        
};

#endif