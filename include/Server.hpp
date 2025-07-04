#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include "Client.hpp"
#include "Request.hpp"
#include "ConfigParser.hpp"

#define BLUE    "\33[34m"
#define GREEN   "\33[32m"
#define RED     "\33[31m"
#define WHITE   "\33[97m"
#define YELLOW  "\33[33m"
#define CYAN    "\33[36m"
#define MAGENTA "\33[35m"
#define GREY    "\33[90m"
#define RESET   "\33[0m" // No Colour

// Forward declaration
struct	serverLevel;
class	Webserv;
class	Request;
class	Client;

class Server {
    public:
        Server(ConfigParser confs, int nbr, Webserv& webserv);
		Server(const Server& copy);
		Server &operator=(const Server& copy);
        ~Server();
        
        Webserv             		&getWebServ();
        struct sockaddr_in  		&getAddr();
        int                 		&getFd();
		std::vector<serverLevel> 	&getConfigs();
		ConfigParser				&getConfParser();
        std::string   				getUploadDir(Client& client, Request& req);
        std::string					getWebRoot(Request& req, locationLevel& loc);
        int                 		openSocket();
        int                 		setOptional();
        int                 		setServerAddr();
        int                 		ft_bind();
        int                 		ft_listen();

    private:
        int                 		_fd;
        struct sockaddr_in  		_addr;
		ConfigParser	    		_confParser;
        std::vector<serverLevel> 	_configs;
        Webserv             		*_webserv;
};

#endif