#ifndef CONFIG_HPP
#define CONFIG_HPP

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
	std::string										port;//listen //int??
	std::string									servName;//server_name
	std::map<int, std::string>					errPages;//error_page
	std::string									maxRequestSize;//client_max_body_size //TODO: maybe size_t??
	std::map<std::string, struct locationLevel>	locations;//location
};

class Config {
    public:
		Config(const std::string& filepath);
		Config(const Config& other);
		Config &operator=(const Config& other);
		~Config();
		// std::map<std::string, std::string> const &getConfig() const;
		void storeConfigs();
		std::vector<std::vector<std::string> > getStoredConfigs();
		void setServerLevel(struct serverLevel& serv, std::vector<std::string>& conf);
		void setLocationLevel(size_t& i, struct serverLevel& serv, std::vector<std::string>& conf, std::vector<std::string>& s);
		void parseAndSetConfigs();
		// void setConfig();
		// const std::string& getConfigValue(const std::string& key) const;
		// void printConfig();
		void printAllConfigs();
		std::vector<std::string> split(std::string& s);
		bool isValidDir(const std::string& path);

    private:
		Config();
		std::string _filepath;
		std::vector<std::vector<std::string> > _storedConfigs;
		std::vector<struct serverLevel> _allConfigs;
        struct serverLevel _config;
        // void parseFile(std::string const &filepath);
};

#endif

/*
	server config class:
		->base:
			listen/port
			server_name
			error_page
			client_max_body_size
			
		->location /:
			root
			index
			methods
			autoindex

		->location /uploads:
			root
			methods
			upload_store
		
		->location ~ \.php$: 
			root
			cgi_pass
			methods
*/