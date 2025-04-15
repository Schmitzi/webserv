#include "../include/Config.hpp"

Config::Config() {}

Config::Config(ConfigParser confs, size_t nbr) {
	if (nbr >= confs.getAllConfigs().size()) {
		std::cerr << "Config could not be constructed because number was out of bounds of all the stored configs\n";
		return;
	}
	_config = confs.getAllConfigs()[nbr];
}

Config::Config(const Config& other) {
	*this = other;
}

Config &Config::operator=(const Config& other) {
	if (this != &other) {
		_config = other._config;
	}
	return *this;
}

Config::~Config() {}

int Config::getPort() {
	
	return _config.port;
}

/* ************************************************************************************** */
