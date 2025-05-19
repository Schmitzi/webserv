#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <iostream>
#include "ConfigParser.hpp"
#include "Server.hpp"
#include "Webserv.hpp"

struct serverLevel;
struct locationLevel;

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

bool						matchLocation(const std::string& path, const serverLevel& serv, locationLevel& bestMatch);
bool						matchUploadLocation(const std::string&, const serverLevel& serv, locationLevel& bestMatch);
bool						matchRootLocation(const std::string& uri, serverLevel& serv, locationLevel& bestMatch);
std::string					resolveFilePathFromUri(const std::string& uri, serverLevel& serv);
const std::string			getStatusMessage(int code);
void						generateErrorPage(std::string& body, int statusCode, const std::string& statusText);
std::string					findErrorPage(int statusCode, Server& server, const std::string& dir);
void						resolveErrorResponse(int statusCode, Server& server, std::string& statusText, std::string& body);


#endif