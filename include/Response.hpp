#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <iostream>
#include "ConfigParser.hpp"
#include "Server.hpp"
#include "Webserv.hpp"
#include "Request.hpp"

struct	serverLevel;
struct	locationLevel;
class	Server;
class	Request;

struct HttpErrorFormat {
	int code;
	const std::string message;
};

static const HttpErrorFormat httpErrors[] = {
	{100, "Continue"},
	{101, "Switching Protocols"},
	{200, "OK"},
	{201, "Created"},
	{202, "Accepted"},
	{204, "No Content"},
	{301, "Moved Permanently"},
	{302, "Found"},
	{303, "See Other"},
	{304, "Not Modified"},
	{307, "Temporary Redirect"},
	{308, "Permanent Redirect"},
	{400, "Bad Request"},
	{401, "Unauthorized"},
	{403, "Forbidden"},
	{404, "Not Found"},
	{405, "Method Not Allowed"},
	{408, "Request Timeout"},
	{409, "Conflict"},
	{413, "Payload Too Large"},
	{414, "URI Too Long"},
	{415, "Unsupported Media Type"},
	{429, "Too Many Requests"},
	{500, "Internal Server Error"},
	{501, "Not Implemented"},
	{502, "Bad Gateway"},
	{503, "Service Unavailable"},
	{504, "Gateway Timeout"},
	{505, "HTTP Version Not Supported"}
};

bool						matchLocation(const std::string& path, const serverLevel& serv, locationLevel*& bestMatch);
bool						matchUploadLocation(const std::string&, const serverLevel& serv, locationLevel*& bestMatch);
const std::string			getStatusMessage(int code);
void						generateErrorPage(std::string& body, int statusCode, const std::string& statusText);
std::string					findErrorPage(int statusCode, const std::string& dir, Request& req);
void						resolveErrorResponse(int statusCode, std::string& statusText, std::string& body, Request& req);


#endif