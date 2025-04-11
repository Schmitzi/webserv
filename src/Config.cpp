#include "../include/Config.hpp"

Config::Config() {
	
}

Config::Config(std::string const &filepath) {
	std::string s = filepath.substr(strlen(filepath.c_str()) - 5, 5);
	if (s != ".conf") {
		std::cerr << "Invalid config file" << std::endl;
		return;
	}
	_filepath = filepath;
	setConfig();
	printConfig();
}

Config::Config(const Config& other) {
	*this = other;
}

Config &Config::operator=(const Config& other) {
	if (this != &other) {
		_filepath = other._filepath;
		_config = other._config;
	}
	return *this;
}

Config::~Config() {
	_config.clear();
}

/* ************************************************************************************** */

std::map<std::string, std::string> const &Config::getConfig() const {
	return _config;
}

void Config::setConfig() {
	std::ifstream file(_filepath.c_str());
	if (!file.is_open()) {
		std::cerr << "Failed to open file." << std::endl;
		return;
	}
	std::string line;
	while (std::getline(file, line)) {
		size_t start = 0;
		size_t end = 0;
		std::string key;
		std::string value;
		if (line.find("root") != std::string::npos) {
			start = line.find("root ") + 5;
			end = strlen(line.c_str()) - start;
			value = line.substr(start, end);
			_config.insert(std::pair<std::string, std::string>("root", value));
		}
		else if (line.find("port") != std::string::npos) {
			start = line.find("port ") + 5;
			end = strlen(line.c_str()) - start;
			value = line.substr(start, end);
			_config.insert(std::pair<std::string, std::string>("port", value));
		}
		else if (line.find("index") != std::string::npos) {
			start = line.find("index ") + 6;
			end = strlen(line.c_str()) - start;
			value = line.substr(start, end);
			_config.insert(std::pair<std::string, std::string>("index", value));
		}
	}
	file.close();
}

const std::string& Config::getConfigValue(const std::string& key) const {
	std::map<std::string, std::string> tmp = getConfig();
	std::map<std::string, std::string>::const_iterator it = tmp.find(key);
	if (it != tmp.end())
		return it->second;
	static const std::string empty = "";
	return empty;
}

void Config::printConfig() {
	std::map<std::string, std::string>::iterator it = _config.begin();
	while (it != _config.end()) {
		std::cout << it->first << " = " << it->second << std::endl;
		++it;
	}
}

// std::vector<std::string> tokenize(const std::string &input) {
// 	std::vector<std::string> tokens;
// 	std::string token;
// 	for (size_t i = 0; i < input.length(); ++i) {
// 		char c = input[i];
// 		if (std::isspace(c)) {
// 			if (!token.empty()) {
// 				tokens.push_back(token);
// 				token.clear();
// 			}
// 		} else if (c == '{' || c == '}' || c == ';') {
// 			if (!token.empty()) {
// 				tokens.push_back(token);
// 				token.clear();
// 			}
// 			tokens.push_back(std::string(1, c));
// 		} else if (c == '#')
// 			while (i < input.length() && input[i] != '\n') ++i;
// 		else
// 			token += c;
// 	}
// 	if (!token.empty())
// 		tokens.push_back(token);
// 	return tokens;
// }
