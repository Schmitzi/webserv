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

        void            			setEnvironment(char **envp);
        void            			flipState();
        Server        				*findServerByFd(int fd);
		Client						*findClientByFd(int fd);
        void            			handleErrorEvent(int fd);
        int             			addToEpoll(int fd, short events);
        void            			removeFromEpoll(int fd);
        void            			handleClientDisconnect(int fd);
        void            			initialize();
		bool						checkEventMaskErrors(uint32_t &eventMask, int &fd);
        int             			run();
        void            			handleNewConnection(Server& server, u_int32_t eventMask);
        void            			handleClientActivity(int clientFd);
		void						handleEpollOut(int fd);
        void            			cleanup();
		int							getEpollFd();
		void						setEpollEvents(int fd, uint32_t events);
		void						addSendBuf(int fd, const std::string& s);
		void						clearSendBuf(int fd);
		const std::string&			getSendBuf(int fd);
		bool						isCgiPipeFd(int fd) const;
		void						registerCgiPipe(int fd, CGIHandler* handler);
		void						unregisterCgiPipe(int fd);
		CGIHandler*					getCgiHandler(int fd) const;

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