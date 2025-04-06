#include "../include/Config.hpp"

Config::Config() {

}

Config::Config(std::string const &filepath) {
	(void)filepath;
}

Config &Config::operator=(Config const &other) {
	_config = other._config;
	return *this;
}

Config::~Config() {

}

std::map<std::string, std::string> const &Config::getConfig() const {
	return _config;
}