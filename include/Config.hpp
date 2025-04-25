#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "ConfigParser.hpp"

class ConfigParser;

class Config {
	private:
		Config();
		struct serverLevel _config;

    public:
		Config(ConfigParser confs, size_t nbr = 0);
		Config(const Config& copy);
		Config &operator=(const Config& copy);
		~Config();
		
		//getters
		int getPort();
		struct serverLevel getConfig();
		
		//extras
		void printConfig();
};

#endif
