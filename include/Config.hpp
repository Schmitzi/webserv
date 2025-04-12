#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <iostream>
#include <cstring>
#include <map>
#include <string>
#include <fstream>
#include <vector>

// typedef struct serverLevel {
// 	size_t port;
// 	std::string servName;
// 	std::map<int, std::string> errPages;
// 	size_t maxRequestSize;
// };

// typedef struct locationLevel {
// 	std::string docRootDir;
// 	std::string indexFile;
// 	std::vector<std::string> methods;
// 	bool autoindex;
// 	std::string redirectionHTTP;
// 	std::string cgiProcessorPath;
// 	std::string uploadDirPath;
// };

class Config {
    public:
		Config(const std::string& filepath);
		Config(const Config& other);
		Config &operator=(const Config& other);
		~Config();
		std::map<std::string, std::string> const &getConfig() const;
		void setConfig();
		const std::string& getConfigValue(const std::string& key) const;
		void printConfig();

    private:
		Config();
		std::string _filepath;
		
        std::map<std::string, std::string> _config;
        void parseFile(std::string const &filepath);
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