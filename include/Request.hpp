#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <iostream>
#include <unistd.h>
#include <map>
#include <sstream>
#include <cstdlib>
#include <iomanip>
#include "ConfigParser.hpp"
// #include "Server.hpp"

struct	serverLevel;
struct	locationLevel;
class	Server;
class	Client;

class Request {
    public:
        Request();
		Request(const Request& copy);
		Request &operator=(const Request& copy);
        Request(const std::string& rawRequest, Client& client, int clientFd);
        ~Request();

        std::string							&getPath();
        std::string const 					&getMethod();
		std::string const					&getCheck();
        std::string const 					&getVersion();
        std::string const 					&getBody();
        std::string const 					&getContentType();
        std::string const 					&getQuery();
        std::string const 					&getBoundary();
        unsigned long     					&getContentLength();
		serverLevel 	  					&getConf();
        std::map<std::string, std::string>	&getHeaders();
        std::string							getMimeType(std::string const &path);
        void    							setPath(std::string const path);
        void    							setBody(std::string const body);
        void    							setContentType(std::string const content);
		bool    							matchHostServerName();
		bool								hasServerName();
        void    							parse(const std::string& rawRequest);
        bool   								parseHeaders(const std::string& headerSection);
        void    							checkContentLength(std::string buffer);
        void    							parseContentType();
        bool    							isChunkedTransfer() const;

    private:
		std::string							_host; 
        std::string                         _method;
		std::string							_check;
        std::string                         _path;
        std::string                         _contentType;
        std::string                         _version;
        std::map<std::string, std::string>  _headers;
        std::string                         _body;
        std::string                         _query;
        std::string                         _boundary;
        unsigned long                       _contentLength;
		serverLevel							_curConf;
		Client								*_client;
		std::vector<serverLevel>			_configs;
		int									_clientFd;
};

#endif