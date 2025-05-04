#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <netinet/in.h>
#include <iostream>
#include <cstring>
#include <map>
#include "../include/Helper.hpp"
#include "../include/Request.hpp"
#include "../include/CGIHandler.hpp"
#include "../include/Multipart.hpp"
#include "../include/Config.hpp"

#define BLUE    "\33[34m"
#define GREEN   "\33[32m"
#define RED     "\33[31m"
#define WHITE   "\33[97m"
#define RESET   "\33[0m" // No Colour

#define MAX_BUFFER_SIZE  100 * 1024 * 1024

// Forward declarations
class Webserv;
class Server;

class Client {
    public:
        Client();
        Client(Webserv &other);
        ~Client();
        struct sockaddr_in      &getAddr();
        socklen_t               &getAddrLen();
        int                     &getFd();
        unsigned char           &getIP();
        char                    &getBuffer();
        void                    setWebserv(Webserv *webserv);
        void                    setServer(Server *server);
        int                     acceptConnection();
        void                    displayConnection();
        int                     recieveData();
        int                     processRequest(char *buffer);
        Request                 parseRequest(char* buffer);
        int                     handleGetRequest(Request& req);
        std::string             extractFileName(const std::string& path);
        int                     handlePostRequest(Request& req);
        int                     handleDeleteRequest(Request& req);
        int                     handleMultipartPost(Request& req);
        ssize_t                 sendResponse(Request req, std::string connect, std::string body);
        void                    sendErrorResponse(int statusCode, const std::string& message);
        bool                    ensureUploadDirectory();
        bool                    saveFile(const std::string& filename, const std::string& content);
        Server*                 matchServerByHost(const std::string& host);
        void                    freeTokens(char **tokens);
    private:
        struct sockaddr_in  _addr;
        socklen_t           _addrLen;
        int                 _fd;
        unsigned char       *_ip;
        char                _buffer[16384];
        std::string         _requestBuffer;
        
        Webserv             *_webserv;
        Server              *_server;
        CGIHandler          _cgi;
};

//create buffer

#endif