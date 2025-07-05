#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <netinet/in.h>
#include <iostream>
#include <cstring>
#include <map>
#include <dirent.h>
#include <fcntl.h>
#include "Response.hpp"
#include "Helper.hpp"
#include "Request.hpp"
#include "CGIHandler.hpp"
#include "Multipart.hpp"
#include "ConfigParser.hpp"
#include "NoErrNo.hpp"

#define BLUE    "\33[34m"
#define GREEN   "\33[32m"
#define RED     "\33[31m"
#define WHITE   "\33[97m"
#define YELLOW  "\33[33m"
#define CYAN    "\33[36m"
#define MAGENTA "\33[35m"
#define GREY    "\33[90m"
#define RESET   "\33[0m" // No Colour

// Forward declarations
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

        int							&getFd();
        Server                  	&getServer();
		std::vector<serverLevel>	getConfigs();
        void                    	setWebserv(Webserv &webserv);
        void                    	setServer(Server &server);
        void                    	setConfigs(const std::vector<serverLevel> &configs);
        int                     	acceptConnection(int serverFd);
        void                    	displayConnection();
        int                     	receiveData();
        int                     	checkLength(bool &printNewLine);
        int                     	processRequest(Request& req);
        int                     	handleGetRequest(Request& req);
        bool                    	isFileBrowserRequest(const std::string& path);
        int                     	handleFileBrowserRequest(Request& req);
		bool						isCGIScript(const std::string& path);
        int                     	handleRegularRequest(Request& req);
        int                     	buildBody(Request &req, std::string fullPath);
		std::string			 		getLocationPath(Request& req, const std::string& method);
        int                     	handlePostRequest(Request& req);
        int                     	handleDeleteRequest(Request& req);
        int                     	handleMultipartPost(Request& req);
        bool                    	ensureUploadDirectory(Request& req);
        bool                    	saveFile(Request& req, const std::string& filename, const std::string& content);
        int                     	viewDirectory(std::string fullPath, Request& req);
        int                     	createDirList(std::string fullPath, Request& req);
        std::string             	showDir(const std::string& dirPath, const std::string& requestUri);
        int                     	handleRedirect(Request eq);
        void                    	sendRedirect(int statusCode, const std::string& location);
        ssize_t                 	sendResponse(Request req, std::string connect, std::string body);
        void                    	sendErrorResponse(int statusCode, Request& req);
        std::string             	decodeChunkedBody(const std::string& chunkedData);
        bool                    	isChunkedRequest(const Request& req);
        bool                    	isChunkedBodyComplete(const std::string& buffer);
		size_t						&getOffset();
		std::string					getConnect();
		void						setConnect(std::string connect);
		int							getExitCode();
		void						setExitCode(int i);

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