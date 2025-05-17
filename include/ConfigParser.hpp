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
#include "Helper.hpp"
#include "ConfigHelper.hpp"

#define BLUE    "\33[34m"
#define GREEN   "\33[32m"
#define RED     "\33[31m"
#define WHITE   "\33[97m"
#define RESET   "\33[0m" // No Colour

struct locationLevel {
	std::string									locName;//location
	std::string									rootLoc;//root
	std::string									indexFile;//index
	std::vector<std::string>					methods;//methods
	bool										autoindexFound;//TODO: can be deleted, just for printing in config.cpp
	bool										autoindex;//autoindex
	std::string									redirectionHTTP;//redirect
	std::string									cgiProcessorPath;//cgi_pass
	std::string									uploadDirPath;//upload_store
};

struct serverLevel {
	std::vector<std::pair<std::pair<std::string, int>, bool> >	port;//listen
	std::string									rootServ;//root
	std::string									indexFile;//index
	std::vector<std::string>					servName;//server_name
	std::map<std::vector<int>, std::string>		errPages;//error_page
	std::string									maxRequestSize;//client_max_body_size
	size_t										requestLimit;//converted maxRequestSize
	std::map<std::string, locationLevel>		locations;//location
};

class ConfigParser {
	private:
		std::string _filepath;
		std::vector<std::vector<std::string> > _storedConfigs;
		std::vector<serverLevel> _allConfigs;
		std::map<std::pair<std::pair<std::string, int>, bool>, std::vector<serverLevel*> > _ipPortToServers;

	public:
		ConfigParser();
		ConfigParser(const std::string& filepath);
		ConfigParser(const ConfigParser& copy);
		ConfigParser &operator=(const ConfigParser& copy);
		~ConfigParser();
		
		//setters
		void storeConfigs();
		void setLocationLevel(size_t& i, std::vector<std::string>& s, serverLevel& serv, std::vector<std::string>& conf);
		void setServerLevel(size_t& i, std::vector<std::string>& s, serverLevel& serv, std::vector<std::string>& conf);
		void setConfigLevels(serverLevel& serv, std::vector<std::string>& conf);
		void setIpPortToServers();
		void parseAndSetConfigs();

		//getters
		std::vector<std::vector<std::string> > getStoredConfigs();
		std::vector<serverLevel> getAllConfigs();

		//extras
		void printAllConfigs();
		void printIpPortToServers();
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