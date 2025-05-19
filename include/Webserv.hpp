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
#include <signal.h>
#include <map>
#include "../include/Config.hpp"

// Forward declarations
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

        Server          &getServer(int i);
        std::vector<Server> &getAllServers();
        std::vector<struct pollfd> &getPfds();
        void            setEnvironment(char **envp);
        char            **getEnvironment() const;
        int             setConfig(std::string const filepath);
        // Polling
        int             addToPoll(int fd, short events);
        void            removeFromPoll(size_t index);
        int             run();
        void            handleNewConnection(Server& server);
        void            handleClientActivity(size_t pollIndex);
        void            ft_error(std::string const msg);
        std::string     getTimeStamp();
        void            printMsg(const std::string msg, char const *colour, std::string const opt);
        Config          &getDefaultConfig();

    private:
        std::vector<Server *> 		_servers;
        std::vector<Client *>   	_clients;
        std::vector<struct pollfd> 	_pfds;
        char                    	**_env;
        ConfigParser				_confParser;
        std::vector<serverLevel>	_configs;


        
};

#endif