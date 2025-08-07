#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <iostream>
#include <cstring>
#include <map>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <set>
#include <utility>
#include "Colors.hpp"

struct locationLevel {
	locationLevel();
	~locationLevel();
	std::string													locName;//location
	std::string													rootLoc;//root
	std::string													indexFile;//index
	std::vector<std::string>									methods;//limit_except
	bool														autoindex;//autoindex
	std::pair<int, std::string>									redirectionHTTP;//redirect
	bool														hasRedirect;
	std::string													cgiProcessorPath;//cgi_pass
	std::string													uploadDirPath;//upload_store
	bool														isRegex;
};

struct serverLevel {
	serverLevel();
	~serverLevel();
	std::vector<std::pair<std::pair<std::string, int>, bool> >	port;//listen
	std::string													rootServ;//root
	std::string													indexFile;//index
	std::vector<std::string>									servName;//server_name
	std::map<std::vector<int>, std::string>						errPages;//error_page
	std::string													maxRequestSize;//client_max_body_size
	size_t														requestLimit;//converted maxRequestSize
	std::map<std::string, locationLevel>						locations;//location
};


class ConfigParser {
	private:
		std::string												_filepath;
		std::vector<std::vector<std::string> >					_storedConfigs;//stores raw config file lines
		std::vector<serverLevel>								_allConfigs;//stores actual server configs!

	public:
		ConfigParser();
		ConfigParser(const std::string& filepath);
		ConfigParser(const ConfigParser& copy);
		ConfigParser &operator=(const ConfigParser& copy);
		~ConfigParser();
		
		//setters
		void													storeConfigs();
		void													setLocationLevel(size_t& i, std::vector<std::string>& s, serverLevel& serv, std::vector<std::string>& conf);
		void													setServerLevel(size_t& i, std::vector<std::string>& s, serverLevel& serv, std::vector<std::string>& conf);
		void													setConfigLevels(serverLevel& serv, std::vector<std::string>& conf);
		void													parseAndSetConfigs();

		//getters
		std::vector<serverLevel>								getAllConfigs();
		int														getPort(serverLevel& conf);
		std::pair<std::pair<std::string, int>, bool>			getDefaultPortPair(serverLevel& conf);
		serverLevel&											getConfigByIndex(size_t nbr);
};

class configException : public std::exception {
	private:
		std::string _message;
	
	public:
		configException() : _message("Unknown configuration error") {}
		configException(const std::string& message) : _message(message) {}
		virtual const char* what() const throw() {
			return _message.c_str();
		};
		virtual ~configException() throw() {};
};

#endif
