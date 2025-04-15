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
	std::string									docRootDir;//root
	std::string									indexFile;//index
	std::vector<std::string>					methods;//methods
	bool										autoindex;//autoindex
	std::string									redirectionHTTP;//redirect
	std::string									cgiProcessorPath;//cgi_pass
	std::string									uploadDirPath;//upload_store
};

struct serverLevel {
	int											port;//listen
	std::string									servName;//server_name
	std::map<int, std::string>					errPages;//error_page
	std::string									maxRequestSize;//client_max_body_size //TODO: maybe size_t??
	std::map<std::string, struct locationLevel>	locations;//location
};

class ConfigParser {
	public:
		ConfigParser();
		ConfigParser(const std::string& filepath);
		ConfigParser(const ConfigParser& other);
		ConfigParser &operator=(const ConfigParser& other);
		~ConfigParser();

		void printAllConfigs();
		void storeConfigs();
		std::vector<std::vector<std::string> > getStoredConfigs();
		std::vector<struct serverLevel> getAllConfigs();
		
		void setConfigLevels(struct serverLevel& serv, std::vector<std::string>& conf);
		void setServerLevel(size_t& i, std::vector<std::string>& s, struct serverLevel& serv, std::vector<std::string>& conf);
		void setLocationLevel(size_t& i, std::vector<std::string>& s, struct serverLevel& serv, std::vector<std::string>& conf);
		void parseAndSetConfigs();

		bool whiteLine(std::string& line);
		bool checkSemicolon(std::string& line);
		std::string skipComments(std::string& s);
		bool isValidDir(const std::string& path);

	private:

		std::string _filepath;
		std::vector<std::vector<std::string> > _storedConfigs;
		std::vector<struct serverLevel> _allConfigs;
};

#endif