#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <netinet/in.h>
#include <iostream>
#include <string>
#include <map>
#include <dirent.h>
#include <fcntl.h>
#include <vector>
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
		size_t						&getOffset();
		std::string					getConnect();
		int							getExitCode();
		std::vector<serverLevel>	getConfigs();
		std::string					&getRequestBuffer();
		void						setWebserv(Webserv &webserv);
		void						setServer(Server &server);
		void						setConfigs(const std::vector<serverLevel> &configs);
		void						setConnect(std::string connect);
		void						setExitCode(int i);

		int							acceptConnection(int serverFd);
		void						displayConnection();
		void						recieveData();
		int							processRequest(Request& req);

		int							handleGetRequest(Request& req);
		int							handlePostRequest(Request& req);
		int							handleDeleteRequest(Request& req);

		int							handleFileBrowserRequest(Request& req);
		int							handleRegularRequest(Request& req);
		int							handleMultipartPost(Request& req);
		int							handleRedirect(Request eq);

		int							viewDirectory(std::string fullPath, Request& req);
		int							createDirList(std::string fullPath, Request& req);
		std::string					showDir(const std::string& dirPath, const std::string& requestUri);
		bool						saveFile(Request& req, const std::string& filename, const std::string& content);

	private:
		struct sockaddr_in  		_addr;
		socklen_t           		_addrLen;
		int                 		_fd;
		std::string         		_requestBuffer;
		Webserv             		*_webserv;
		Server              		*_server;
		std::vector<serverLevel>	_configs;
		size_t						_sendOffset;
		std::string					_connect;
		int							_exitCode;
};

#endif