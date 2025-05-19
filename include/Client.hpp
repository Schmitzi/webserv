#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <netinet/in.h>
#include <iostream>
#include <cstring>
#include <map>
#include <dirent.h>
#include <fcntl.h>
#include "../include/Response.hpp"
#include "../include/Helper.hpp"
#include "../include/Request.hpp"
#include "../include/CGIHandler.hpp"
#include "../include/Multipart.hpp"
#include "../include/Config.hpp"
#include "../include/ConfigParser.hpp"

#define BLUE    "\33[34m"
#define GREEN   "\33[32m"
#define RED     "\33[31m"
#define WHITE   "\33[97m"
#define RESET   "\33[0m" // No Colour

// Forward declarations
class Webserv;
class Server;
class Request;
struct serverLevel;
struct locationLevel;

class Client {
    public:
        Client();
        Client(Server& serv);
        ~Client();
        struct sockaddr_in      &getAddr();
        socklen_t               &getAddrLen();
        int                     &getFd();
        unsigned char           &getIP();
        char                    &getBuffer();
        Webserv					&getWebserv();
        Server                  &getServer();
        void                    setWebserv(Webserv *webserv);
        void                    setServer(Server *server);
        void                    setConfig(serverLevel config);
        void                    setAutoIndex();
        int                     acceptConnection(int serverFd);
        void                    displayConnection();
        int                     recieveData();
        int                     processRequest(char *buffer);
        Request                 parseRequest(char* buffer);
        int                     handleGetRequest(Request& req);
        bool                    isFileBrowserRequest(const std::string& path);
        int                     handleFileBrowserRequest(Request& req, const std::string& requestPath);
        int                     handleRegularRequest(Request& req, const std::string& requestPath);
        int                     buildBody(Request &req, std::string fullPath);
		std::string			 	getLocationPath(Request& req, const std::string& method);
        std::string             extractFileName(const std::string& path);
        int                     handlePostRequest(Request& req);
        int                     handleDeleteRequest(Request& req);
        int                     handleMultipartPost(Request& req);
        bool                    ensureUploadDirectory(Request& req);
        bool                    saveFile(Request& req, const std::string& filename, const std::string& content);
        int                     viewDirectory(std::string fullPath, std::string requestPath);
        int                     createDirList(std::string fullPath, std::string requestPath);
        std::string             showDir(const std::string& dirPath, const std::string& requestUri);
        int                     handleRedirect(Request eq);
        void                    sendRedirect(int statusCode, const std::string& location);

        void                    findContentType(Request &req);
        ssize_t                 sendResponse(Request req, std::string connect, std::string body);
        void                    sendErrorResponse(int statusCode);
        bool					send_all(int sockfd, const std::string& data);
    private:
        struct sockaddr_in  _addr;
        socklen_t           _addrLen;
        int                 _fd;
        unsigned char       *_ip;
        char                _buffer[16384];
        std::string         _requestBuffer;
        bool                _autoindex;
        
        Webserv             *_webserv;
        Server              *_server;
        CGIHandler          _cgi;
        serverLevel         _config;
};

//create buffer

#endif