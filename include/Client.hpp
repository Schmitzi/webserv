#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <netinet/in.h>
#include <iostream>
#include <cstring>
#include <map>
#include "../include/Helper.hpp"
#include "../include/Request.hpp"
#include "../include/CGIHandler.hpp"

#define BLUE    "\33[34m"
#define GREEN   "\33[32m"
#define RED     "\33[31m"
#define WHITE   "\33[97m"
#define RESET   "\33[0m" // No Colour

class Webserv;
class Server;
class CGIHandler;

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
        std::string const       &getRawData();
        void                    setWebserv(Webserv *webserv);
        void                    setServer(Server *server);
        void                    setRawData(std::string const data);
        int                     acceptConnection();
        void                    displayConnection();
        int                     recieveData();
        int                     processRequest(char const *buffer);
        Request                 parseRequest(char const *buffer);
        int                     handleGetRequest(Request& req);
        std::string             extractFileName(const std::string& path);
        int                     handlePostRequest(Request& req);
        int                     handleDeleteRequest(Request& req);
        ssize_t                 sendResponse(Request req, std::string connect, std::string body);
        void                    sendErrorResponse(int statusCode, const std::string& message);
        int                     handleMultipartPost(Request& req);
        bool                    isRequestComplete(std::string &data);
        void                    freeTokens(char **tokens);
    private:
        struct sockaddr_in  _addr;
        socklen_t           _addrLen;
        int                 _fd;
        unsigned char       *_ip;
        char				_buffer[1024];
        std::string         _rawData;

        Webserv             *_webserv;
        Server              *_server;
        CGIHandler          _cgi;
};

#endif