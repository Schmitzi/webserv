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
	int											port;//listen
	std::string									rootServ;//root
	std::string									indexFile;
	std::string									servName;//server_name
	std::map<std::vector<int>, std::string>		errPages;//error_page
	std::string									maxRequestSize;//client_max_body_size
	size_t										requestLimit;//converted maxRequestSize
	// data_directory?
	std::map<std::string, struct locationLevel>	locations;//location
};

class ConfigParser {
	public:
		ConfigParser();
		ConfigParser(const std::string& filepath);
		ConfigParser(const ConfigParser& other);
		ConfigParser &operator=(const ConfigParser& other);
		~ConfigParser();

		//extras
		void printAllConfigs();

		//small checks and skipping stuff
		bool onlyDigits(const std::string& s);
		bool whiteLine(std::string& line);
		bool checkSemicolon(std::string& line);
		std::string skipComments(std::string& s);

		//check if valid path/dir/file...
		bool isValidPath(const std::string& path);
		bool isValidDir(const std::string& path);
		bool isValidName(const std::string& name);
		bool isValidIndexFile(const std::string& indexFile);
		void parseClientMaxBodySize(struct serverLevel& serv);

		//check if valid config
		void checkRoot(struct serverLevel& serv);
		void checkIndex(struct serverLevel& serv);
		void checkConfig(struct serverLevel& serv);
		
		//setters
		void storeConfigs();
		void setLocationLevel(size_t& i, std::vector<std::string>& s, struct serverLevel& serv, std::vector<std::string>& conf);
		void setServerLevel(size_t& i, std::vector<std::string>& s, struct serverLevel& serv, std::vector<std::string>& conf);
		void setConfigLevels(struct serverLevel& serv, std::vector<std::string>& conf);
		void parseAndSetConfigs();

		//getters
		std::vector<std::vector<std::string> > getStoredConfigs();
		std::vector<struct serverLevel> getAllConfigs();
		
	private:

		std::string _filepath;
		std::vector<std::vector<std::string> > _storedConfigs;
		std::vector<struct serverLevel> _allConfigs;
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