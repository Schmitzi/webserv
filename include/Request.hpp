#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <iostream>
#include <unistd.h>
#include <map>
#include <sstream>
#include <cstdlib>
#include <iomanip>
#include <string>
#include <vector>
#include <cctype>
#include "ConfigParser.hpp"

class	Server;
class	Client;
struct	serverLevel;
struct	locationLevel;

class Request {
	public:
		Request();
		Request(const std::string& rawRequest, Client& client, int clientFd);
		Request(const Request& copy);
		Request &operator=(const Request& copy);
		~Request();

		//getters & setters
		bool								&init();
		std::string							&getPath();
		std::string const					&getMethod();
		std::string const					&getVersion();
		std::string const					&getBody();
		std::string const					&getContentType();
		std::string const					&getQuery();
		std::string const					&getBoundary();
		unsigned long						&getContentLength();

		serverLevel							&getConf();
		std::map<std::string, std::string>	&getHeaders();
		bool								&hasValidLength();
		bool								&isChunked();
		std::string							&check();
		Client								&getClient();
		std::string							getMimeType(std::string const &path);
		
		void								setPath(std::string const path);
		void								setBody(std::string const body);
		void								setContentType(std::string const content);

		bool								hasServerName();
		bool								matchHostServerName();
		bool								miniCheckRaw(std::string& s);
		bool								checkRaw(const std::string& raw);
		void								parse(const std::string& rawRequest);
		void								setHeader(std::string& key, std::string& value, bool ignoreHost);
		bool								checkMethod();
		bool								checkVersion();
		void								checkQueryAndPath(std::string target);
		void								getHostAndPath(std::string& target);
		void								parseHeaders(const std::string& headerSection);
		void								checkContentLength(std::string buffer);
		void								parseContentType();
		bool								isChunkedTransfer();

	private:
		bool								_init;
		std::string							_host; 
		std::string							_method;
		std::string							_check;
		std::string							_path;
		std::string							_contentType;
		std::string							_version;
		std::string							_body;
		std::string							_query;
		std::string							_boundary;
		unsigned long						_contentLength;
		std::map<std::string, std::string>	_headers;
		serverLevel							_curConf;
		Client								*_client;
		std::vector<serverLevel>			_configs;
		int									_clientFd;
		bool								_hasValidLength;
		bool								_isChunked;
};

#endif
