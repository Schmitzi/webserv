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

#define HTTP_STATUS_CODES \
	X(100, Continue, "Continue") \
	X(101, SwitchingProtocols, "Switching Protocols") \
	X(200, OK, "OK") \
	X(201, Created, "Created") \
	X(202, Accepted, "Accepted") \
	X(204, NoContent, "No Content") \
	X(301, MovedPermanently, "Moved Permanently") \
	X(302, Found, "Found") \
	X(303, SeeOther, "See Other") \
	X(304, NotModified, "Not Modified") \
	X(307, TemporaryRedirect, "Temporary Redirect") \
	X(308, PermanentRedirect, "Permanent Redirect") \
	X(400, BadRequest, "Bad Request") \
	X(401, Unauthorized, "Unauthorized") \
	X(403, Forbidden, "Forbidden") \
	X(404, NotFound, "Not Found") \
	X(405, MethodNotAllowed, "Method Not Allowed") \
	X(408, RequestTimeout, "Request Timeout") \
	X(409, Conflict, "Conflict") \
	X(413, PayloadTooLarge, "Payload Too Large") \
	X(414, URITooLong, "URI Too Long") \
	X(415, UnsupportedMediaType, "Unsupported Media Type") \
	X(429, TooManyRequests, "Too Many Requests") \
	X(500, InternalServerError, "Internal Server Error") \
	X(501, NotImplemented, "Not Implemented") \
	X(502, BadGateway, "Bad Gateway") \
	X(503, ServiceUnavailable, "Service Unavailable") \
	X(504, GatewayTimeout, "Gateway Timeout") \
	X(505, HTTPVersionNotSupported, "HTTP Version Not Supported")

enum HttpStatusCode {
	#define X(code, name, str) Http_##name = code,
	HTTP_STATUS_CODES
	#undef X
};

struct HttpErrorFormat {
	int code;
	const std::string name;
	const std::string message;
};

static const HttpErrorFormat httpErrors[] = {
	#define X(code, name, message) {code, #name, message},
	HTTP_STATUS_CODES
	#undef X
};

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
        void                    findContentType(Request &req);
        ssize_t                 sendResponse(Request req, std::string connect, std::string body);
		bool					send_all(int sockfd, const std::string& data);
        void                    sendErrorResponse(int statusCode, const std::string& message);
		const std::string		getStatusMessage(int code);
    private:
        struct sockaddr_in  		_addr;
        socklen_t           		_addrLen;
        int                 		_fd;
        unsigned char       		*_ip;
        char						_buffer[1024];

        Webserv             		*_webserv;
        Server              		*_server;
        CGIHandler          		_cgi;
};

#endif