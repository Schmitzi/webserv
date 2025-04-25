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
#include <stdlib.h>
#include "../include/Helper.hpp"
#include "../include/ConfigHelper.hpp"
#include <algorithm>

/*
	minimum requirements:
	server: 
		-listen
		-server_name?
		-root
		-index
	location: 
		-root (default: from server)
		-index (default: from server)
*/

struct locationLevel {
	std::string									rootLoc;//root
	std::string									indexFile;//index
	std::vector<std::string>					methods;//methods
	bool										autoindexFound;
	bool										autoindex;//autoindex
	std::string									redirectionHTTP;//redirect
	std::string									cgiProcessorPath;//cgi_pass
	std::string									uploadDirPath;//upload_store
	//return(=redirect?), limit_except(=methods?)
};

struct serverLevel {
	std::vector<std::pair<std::string, int> >	port;//listen
	std::string									rootServ;//root
	std::string									indexFile;
	std::vector<std::string>					servName;//server_name
	std::map<std::vector<int>, std::string>		errPages;//error_page
	std::string									maxRequestSize;//client_max_body_size
	size_t										requestLimit;//converted maxRequestSize
	// data_directory?
	std::map<std::string, struct locationLevel>	locations;//location
};

class ConfigParser {
	private:
		std::string _filepath;
		std::vector<std::vector<std::string> > _storedConfigs;
		std::vector<struct serverLevel> _allConfigs;
		std::map<std::pair<std::string, int>, std::vector<struct serverLevel*> > _ipPortToServers;

	public:
		ConfigParser();
		ConfigParser(const std::string& filepath);
		ConfigParser(const ConfigParser& copy);
		ConfigParser &operator=(const ConfigParser& copy);
		~ConfigParser();
		
		//setters
		void storeConfigs();
		void setLocationLevel(size_t& i, std::vector<std::string>& s, struct serverLevel& serv, std::vector<std::string>& conf);
		void setServerLevel(size_t& i, std::vector<std::string>& s, struct serverLevel& serv, std::vector<std::string>& conf);
		void setConfigLevels(struct serverLevel& serv, std::vector<std::string>& conf);
		void parseAndSetConfigs();
		void setIpPortToServers();

		//getters
		std::vector<std::vector<std::string> > getStoredConfigs();
		std::vector<struct serverLevel> getAllConfigs();

		//extras
		void printAllConfigs();
		void printIpPortToServers() const;
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