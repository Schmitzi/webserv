#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <netinet/in.h>
#include <iostream>
#include <string>
#include <map>
#include <dirent.h>
#include <fcntl.h>
#include <vector>
#include <cstdio>
#include "Colors.hpp"

class	Webserv;
class	Server;
class	Request;
struct	serverLevel;
struct	locationLevel;

class Client {
	public:
		Client(Server& serv);
		Client(const Client& client);
		Client &operator=(const Client& other);
		~Client();

		//getters & setters
		int							&getFd();
		Server						&getServer();
		Webserv						&getWebserv();
		Request						&getRequest();
		size_t						&getOffset();
		std::vector<serverLevel>	getConfigs();
		bool						&exitErr();
		bool						&fileIsNew();
		bool						&shouldClose();
		time_t						&lastUsed();
		std::string					&output();
		int							&statusCode();

		int							acceptConnection(int serverFd);
		void						displayConnection();
		void						recieveData();
		int							processRequest();

		int							handleGetRequest();
		int							handlePostRequest();
		int							handleDeleteRequest();

		int							handleFileBrowserRequest();
		int							handleRegularRequest();
		int							handleMultipartPost();
		int							handleRedirect();

		int							viewDirectory(std::string fullPath, Request& req);
		int							createDirList(std::string fullPath, Request& req);
		std::string					showDir(const std::string& dirPath, const std::string& requestUri);
		bool						saveFile(Request& req, const std::string& filename, const std::string& content);

	private:
		struct sockaddr_in  		_addr;
		socklen_t           		_addrLen;
		int                 		_fd;
		std::string         		_requestBuffer;
		Server              		*_server;
		Webserv             		*_webserv;
		Request						*_req;
		std::vector<serverLevel>	_configs;
		size_t						_sendOffset;
		bool						_exitErr;
		bool						_fileIsNew;
		bool						_shouldClose;
		time_t						_lastUsed;
		std::string					_output;
		int							_statusCode;
};

#endif
