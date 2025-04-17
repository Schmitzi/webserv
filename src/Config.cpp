#include "../include/Config.hpp"

Config::Config() {}

Config::Config(ConfigParser confs, size_t nbr) {
	if (nbr >= confs.getAllConfigs().size()) {
		std::cerr << "Config could not be constructed because number was out of bounds\n";
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

/* ************************************************************************************** */
//GETTERS

int Config::getPort() {
	return _config.port;
}

struct serverLevel Config::getConfig() {
	return _config;
}

/* ************************************************************************************** */
//EXTRAS

void Config::printConfig() {
	std::cout << "___config___" << std::endl
	<< "server {" << std::endl
	<< "\troot: " << _config.rootServ << std::endl
	<< "\tport: " << _config.port << std::endl
	<< "\tserver_name: " << _config.servName << std::endl
	<< "\terror_page:" << std::endl;
	std::map<int, std::string>::iterator it = _config.errPages.begin();
	while (it != _config.errPages.end()) {
		std::cout << "\t\t" << it->first << " " << it->second << std::endl;
		++it;
	}
	std::cout << "\tclient_max_body_size: " << _config.maxRequestSize << std::endl << std::endl;
	std::map<std::string, struct locationLevel>::iterator its = _config.locations.begin();
	while (its != _config.locations.end()) {
		std::cout << "\tlocation " << its->first << " {" << std::endl
		<< "\t\troot: " << its->second.rootLoc << std::endl
		<< "\t\tindex: " << its->second.indexFile << std::endl
		<< "\t\tmethods:";
		for (size_t i = 0; i < its->second.methods.size(); i++)
			std::cout << " " << its->second.methods[i];
		std::cout << std::endl << "\t\tautoindex: ";
		if (its->second.autoindex == true)
			std::cout << "on" << std::endl;
		else
			std::cout << "off" << std::endl;
		std::cout << "\t\tredirect: " << its->second.redirectionHTTP << std::endl
		<< "\t\tcgi_pass: " << its->second.cgiProcessorPath << std::endl
		<< "\t\tupload_store: " << its->second.uploadDirPath << std::endl
		<< "\t}" << std::endl << std::endl;
		++its;
	}
	std::cout << "}" << std::endl;
}