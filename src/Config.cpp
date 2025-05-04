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
    // if (_config.port.empty()) {
    //     return 8080; // Default port
    // }
    return _config.port[0].second;
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
			if (_config.port[i].first != "0.0.0.0")
				std::cout << _config.port[i].first << " ";
			std::cout << _config.port[i].second << std::endl;
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

// #include "../include/Config.hpp"

// Config::Config() {
	
// }

// Config::Config(std::string const &filepath) {
// 	std::string s = filepath.substr(strlen(filepath.c_str()) - 5, 5);
// 	if (s != ".conf") {
// 		std::cerr << "Invalid config file" << std::endl;
// 		return;
// 	}
// 	_filepath = filepath;
// 	setConfig();
// 	printConfig();
// }

// Config::Config(const Config& other) {
// 	*this = other;
// }

// Config &Config::operator=(const Config& other) {
// 	if (this != &other) {
// 		_filepath = other._filepath;
// 		_config = other._config;
// 	}
// 	return *this;
// }

// Config::~Config() {
// 	_config.clear();
// }

// /* ************************************************************************************** */

// const std::map<std::string, std::string>& Config::getConfig() const {
//     return _config;
// }

// void Config::setConfig() {
// 	std::ifstream file(_filepath.c_str());
// 	if (!file.is_open()) {
// 		std::cerr << "Failed to open file." << std::endl;
// 		return;
// 	}
// 	std::string line;
// 	while (std::getline(file, line)) {
// 		size_t start = 0;
// 		size_t end = 0;
// 		std::string key;
// 		std::string value;
// 		if (line.find("root") != std::string::npos) {
// 			start = line.find("root ") + 5;
// 			end = strlen(line.c_str()) - start;
// 			value = line.substr(start, end);
// 			_config.insert(std::pair<std::string, std::string>("root", value));
// 		}
// 		else if (line.find("port") != std::string::npos) {
// 			start = line.find("port ") + 5;
// 			end = strlen(line.c_str()) - start;
// 			value = line.substr(start, end);
// 			_config.insert(std::pair<std::string, std::string>("port", value));
// 		}
// 		else if (line.find("index") != std::string::npos) {
// 			start = line.find("index ") + 6;
// 			end = strlen(line.c_str()) - start;
// 			value = line.substr(start, end);
// 			_config.insert(std::pair<std::string, std::string>("index", value));
// 		}
// 	}
// 	file.close();
// }

// const std::string& Config::getConfigValue(const std::string& key) const {
//     // Don't create a copy - just access _config directly
//     std::map<std::string, std::string>::const_iterator it = _config.find(key);
//     if (it != _config.end())
//         return it->second;
//     static const std::string empty = "";
//     return empty;
// }

// void Config::printConfig() {
// 	std::map<std::string, std::string>::iterator it = _config.begin();
// 	while (it != _config.end()) {
// 		std::cout << it->first << " = " << it->second << std::endl;
// 		++it;
// 	}
// }

// // std::vector<std::string> tokenize(const std::string &input) {
// // 	std::vector<std::string> tokens;
// // 	std::string token;
// // 	for (size_t i = 0; i < input.length(); ++i) {
// // 		char c = input[i];
// // 		if (std::isspace(c)) {
// // 			if (!token.empty()) {
// // 				tokens.push_back(token);
// // 				token.clear();
// // 			}
// // 		} else if (c == '{' || c == '}' || c == ';') {
// // 			if (!token.empty()) {
// // 				tokens.push_back(token);
// // 				token.clear();
// // 			}
// // 			tokens.push_back(std::string(1, c));
// // 		} else if (c == '#')
// // 			while (i < input.length() && input[i] != '\n') ++i;
// // 		else
// // 			token += c;
// // 	}
// // 	if (!token.empty())
// // 		tokens.push_back(token);
// // 	return tokens;
// // }
