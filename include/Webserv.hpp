#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <iostream>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <sstream>
#include <ctime>
#include <sys/epoll.h>
#include <iomanip>
#include <vector>
#include <signal.h>
#include <map>
#include "Colors.hpp"
#include "ConfigParser.hpp"

#define MAX_EVENTS 64

class	Server;
class	Client;
class	CGIHandler;

class Webserv {
	public:
		Webserv(std::string config = "config/test.conf");
		Webserv(Webserv const &other);
		Webserv &operator=(Webserv const &other);
		~Webserv();

		//setters & getters
		void						flipState();
		void						setEnvironment(char **envp);
		std::string					&getSendBuf(int fd);
		std::map<int, std::string>	&getSendBuf();
		std::map<int, CGIHandler*>	&getCgis();
		int							getEpollFd();
		CGIHandler					*getCgiHandler(int fd);
		Server						*getServerByFd(int fd);
		Client						*getClientByFd(int fd);

		void						initialize();
		bool						checkEventMaskErrors(uint32_t &eventMask, int fd);
		int							run();
		void						checkClientTimeout(time_t now, Client &client);
		void						handleErrorEvent(int fd);
		void						handleClientDisconnect(int fd);
		void						handleNewConnection(Server& server);
		void						handleClientActivity(int clientFd);
		void						handleEpollOut(int fd);
		void						checkCgiTimeouts();
		void						cleanup();

	private:
		bool						_state;
		std::vector<Server>			_servers;
		std::vector<Client>			_clients;
		std::map<int, CGIHandler*>	_cgis;
		char						**_env;
		int							_epollFd;
		struct epoll_event			_events[MAX_EVENTS];
		ConfigParser				_confParser;
		std::vector<serverLevel>	_configs; 
		std::map<int, std::string>	_sendBuf;
};

#endif