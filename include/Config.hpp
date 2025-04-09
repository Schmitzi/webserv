#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <iostream>
#include <cstring>
#include <map>
#include <string>
#include <fstream>

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
        //void parseFile(std::string const &filepath);
};

#endif
