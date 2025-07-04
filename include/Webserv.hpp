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
#include "CGIHandler.hpp"
#include "Helper.hpp"
#include "EpollHelper.hpp"

// Forward declarations
class	Server;
class	Client;

// COLOURS
#define BLUE    "\33[34m"
#define GREEN   "\33[32m"
#define RED     "\33[31m"
#define WHITE   "\33[97m"
#define YELLOW  "\33[33m"
#define CYAN    "\33[36m"
#define MAGENTA "\33[35m"
#define GREY    "\33[90m"
#define RESET   "\33[0m" // No Colour

// Others
#define MAX_EVENTS 64

class	CGIHandler;

class Webserv {
    public:
        Webserv(std::string config = "config/test.conf");
        Webserv(Webserv const &other);
        Webserv &operator=(Webserv const &other);
        ~Webserv();

        // Getters and Setters
        void            			setEnvironment(char **envp);
        void            			flipState();
        Server*        				getServerByFd(int fd);
		Client*						getClientByFd(int fd);
		int							getEpollFd();
		std::string			        &getSendBuf(int fd);
        std::map<int, std::string>  &getSendBuf();
        std::map<int, CGIHandler *> &getCgis();

        void            			initialize();
        int             			run();

		void						handleEpollOut(int fd);
        void            			handleNewConnection(Server& server);
        void            			handleClientActivity(int clientFd);
        void            			handleClientDisconnect(int fd);

		// bool						checkEventMaskErrors(uint32_t &eventMask, int &fd);
        void            			handleErrorEvent(int fd);
        void            			cleanup();

    private:
        bool                        _state;
        std::vector<Server> 		_servers;
        std::vector<Client>   	    _clients;
		std::map<int, CGIHandler*>	_cgis;
        char                    	**_env;
        int                         _epollFd;
        struct epoll_event          _events[MAX_EVENTS];
        ConfigParser				_confParser;
        std::vector<serverLevel>	_configs; 
		std::map<int, std::string>	_sendBuf;
};

#endif