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
	if (this != &other)
		_config = other._config;
	return *this;
}

Config::~Config() {}

/* ************************************************************************************** */
//GETTERS

int Config::getPort() {
	for (size_t i = 0; i < _config.port.size(); i++) {
		std::pair<std::pair<std::string, int>, bool> ipPort = _config.port[i];
		if (ipPort.second == true)
			return ipPort.first.second;
	}
	return _config.port[0].first.second;
}

std::pair<std::pair<std::string, int>, bool> Config::getDefaultPortPair() {
	for (size_t i = 0; i < _config.port.size(); i++) {
		std::pair<std::pair<std::string, int>, bool> ipPort = _config.port[i];
		if (ipPort.second == true)
			return ipPort;
	}
	return _config.port[0];
}

serverLevel Config::getConfig() {
	return _config;
}

/* ************************************************************************************** */
//EXTRAS

void Config::printConfig() {//only temporary, for debugging
	std::cout << "___config___" << std::endl
	<< "server {" << std::endl;
	if (!_config.rootServ.empty())
		std::cout << "\troot: " << _config.rootServ << std::endl;
	if (!_config.indexFile.empty())
		std::cout << "\tindex: " << _config.indexFile << std::endl;
	if (!_config.port.empty()) {
		std::cout << "\tport:" << std::endl;
		for (size_t i = 0; i < _config.port.size(); i++) {
			std::cout << "\t\t";
			std::pair<std::pair<std::string, int>, bool> ipPort = _config.port[i];
			if (_config.port[i].first.first != "0.0.0.0")
				std::cout << _config.port[i].first.first << " ";
			std::cout << _config.port[i].first.first << std::endl;
			if (_config.port[i].second == true)
				std::cout << "\t\tdefault_server" << std::endl;
		}
	}
	if (!_config.servName.empty()) {
		for (size_t i = 0; i < _config.servName.size(); i++) {
			if (i == 0)
				std::cout << "\tserver_name:";
			std::cout << " " << _config.servName[i];
		}
		std::cout << std::endl;
	}
	std::map<std::vector<int>, std::string>::iterator it = _config.errPages.begin();
	if (it != _config.errPages.end()) {
		std::cout << "\terror_page:" << std::endl;
		while (it != _config.errPages.end()) {
			std::cout << "\t\t";
			for (size_t i = 0; i < it->first.size(); i++)
				std::cout << it->first[i] << " ";
			std::cout << it->second << std::endl;
			++it;
		}
	}
	if (!_config.maxRequestSize.empty())
		std::cout << "\tclient_max_body_size: " << _config.maxRequestSize << std::endl << std::endl;
	std::map<std::string, locationLevel>::iterator its = _config.locations.begin();
	while (its != _config.locations.end()) {
		std::cout << "\tlocation " << its->first << " {" << std::endl;
		if (!its->second.rootLoc.empty())
			std::cout << "\t\troot: " << its->second.rootLoc << std::endl;
		if (!its->second.indexFile.empty())
			std::cout << "\t\tindex: " << its->second.indexFile << std::endl;
		if (its->second.methods.size() > 0) {
			std::cout << "\t\tmethods:";
			for (size_t i = 0; i < its->second.methods.size(); i++)
				std::cout << " " << its->second.methods[i];
		}
		if (its->second.autoindexFound == true) {
			std::cout << std::endl << "\t\tautoindex: ";
			if (its->second.autoindex == true)
				std::cout << "on" << std::endl;
			else
				std::cout << "off" << std::endl;
		}
		if (!its->second.redirectionHTTP.empty())
			std::cout << "\t\tredirect: " << its->second.redirectionHTTP << std::endl;
		if (!its->second.cgiProcessorPath.empty())
			std::cout << "\t\tcgi_pass: " << its->second.cgiProcessorPath << std::endl;
		if (!its->second.uploadDirPath.empty())
			std::cout << "\t\tupload_store: " << its->second.uploadDirPath << std::endl;
		std::cout << "\t}" << std::endl << std::endl;
		++its;
	}
	std::cout << "}" << std::endl;
}
