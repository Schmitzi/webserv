#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "ConfigParser.hpp"

class ConfigParser;

class Config {
    public:
		Config(ConfigParser confs, size_t nbr = 0);//from configparser!
		Config(const Config& other);
		Config &operator=(const Config& other);
		~Config();
		int getPort();

    private:
		Config();
        struct serverLevel _config;
};

#endif
