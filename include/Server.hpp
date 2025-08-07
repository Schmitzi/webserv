#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <vector>
#include "Colors.hpp"
#include "ConfigParser.hpp"

class	Webserv;
class	Request;
class	Client;
struct	serverLevel;
struct	locationLevel;

class Server {
	public:
		Server(ConfigParser confs, int nbr, Webserv& webserv);
		Server(const Server& copy);
		Server &operator=(const Server& copy);
		~Server();
		
		//getters & setters
		Webserv						&getWebServ();
		struct sockaddr_in			&getAddr();
		int							&getFd();
		std::vector<serverLevel>	&getConfigs();
		ConfigParser				&getConfParser();
		std::string					getUploadDir(Client& client, Request& req, std::string dir = "");
		std::string					getWebRoot(Request& req, locationLevel& loc);
		int							setOptional();
		int							setServerAddr();

		int							openSocket();
		int							ft_bind();
		int							ft_listen();

	private:
		int							_fd;
		struct sockaddr_in			_addr;
		ConfigParser				_confParser;
		std::vector<serverLevel>	_configs;
		Webserv						*_webserv;
};

#endif
