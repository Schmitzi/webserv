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
	// setConfig();
	storeConfigs();
	parseAndSetConfigs();
	// printConfig();
}//TODO: ADD FLAG IN WEBSERV FOR SETTING CONFIGS

Config::Config(const Config& other) {
	*this = other;
}

Config &Config::operator=(const Config& other) {
	if (this != &other) {
		_filepath = other._filepath;
		_config = other._config;
		_storedConfigs = other._storedConfigs;
		_allConfigs = other._allConfigs;
	}
	return *this;
}

Config::~Config() {
	// _config.clear();
}

/* ************************************************************************************** */

//parse config, set configs!
void Config::storeConfigs() {
	std::ifstream file(_filepath.c_str());
	if (!file.is_open()) {
		std::cerr << "Failed to open file." << std::endl;
		return;
	}
	std::string line;
	int configNum = -1;
	while (std::getline(file, line)) {
		if (line.find("server {") != std::string::npos) {
			nextConfig:
			configNum++;
			_storedConfigs.resize(configNum + 1);
			_storedConfigs[configNum].push_back(line);
			while (std::getline(file, line) && line.find("server {") == std::string::npos) {
				_storedConfigs[configNum].push_back(line);
			}
			if (line.find("server {") != std::string::npos)
				goto nextConfig;
		}
	}
}

std::vector<std::vector<std::string> > Config::getStoredConfigs() {
	return _storedConfigs;
}

std::vector<std::string> Config::split(std::string& s) {
	std::vector<std::string> ret;
	std::istringstream iss(s);
	std::string single;
	while (iss >> single)
		ret.push_back(single);
	return ret;
}

bool Config::isValidDir(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
        return false;
    if (!S_ISDIR(info.st_mode))
        return false;
    if (access(path.c_str(), R_OK | X_OK) != 0)
        return false;
    return true;
}

void Config::setLocationLevel(size_t& i, struct serverLevel& serv, std::vector<std::string>& conf, std::vector<std::string>& s) {
	struct locationLevel loc;
	loc.autoindex = false;
	std::string locName;
	if (s[0].find("location") != std::string::npos) {
		for (size_t x = 1; x < s.size() && s[x] != "{"; x++)
			locName += s[x];
	}
	else
		locName = "location";
	i++;
	if (i < conf.size() && conf[i].find("}") == std::string::npos) {
		s = split(conf[i]);
		for (size_t y = 0; y < s.size(); y++) {
			std::cout << s[y] << " ";
		}
		std::cout << "___\n";
		size_t a = s.size() - 1;
		size_t b = s[a].size() - 1;
		if (s[a][b] == ';') {
			std::string t = s[a].substr(0, s[a].size() - 1);
			s[a] = t;
		}
		if (s[0].find("root") != std::string::npos) {
			loc.docRootDir = s[1];
			if (!isValidDir(loc.docRootDir))
				std::cerr << "Error: invalid directory path in config file -> " << loc.docRootDir << std::endl;
		}
		else if (s[0].find("index") != std::string::npos)
			loc.indexFile = s[1];
		else if (s[0].find("methods") != std::string::npos) {
			for (size_t m = 1; m < s.size(); m++)
				loc.methods.push_back(s[1]);
		}
		else if (s[0].find("autoindex") != std::string::npos && s[1].find("on") != std::string::npos)
			loc.autoindex = true;
		else if (s[0].find("redirect") != std::string::npos)
			loc.redirectionHTTP = s[1];
		else if (s[0].find("cgi_pass") != std::string::npos) {
			loc.cgiProcessorPath = s[1];
			if (!isValidDir(loc.cgiProcessorPath))
				std::cerr << "Error: invalid directory path in config file -> " << loc.cgiProcessorPath << std::endl;
		}
		else if (s[0].find("upload_store") != std::string::npos) {
			loc.uploadDirPath = s[1];
			if (!isValidDir(loc.uploadDirPath))
				std::cerr << "Error: invalid directory path in config file -> " << loc.uploadDirPath << std::endl;
		}
	}
	serv.locations.insert(std::pair<std::string, struct locationLevel>(locName, loc));
	std::cout << "LOCATION: " << locName << std::endl;
}


void Config::setServerLevel(struct serverLevel& serv, std::vector<std::string>& conf) {
	struct locationLevel loc;
	loc.autoindex = false;
	std::string locName;
	size_t i = 0;
	while (i < conf.size()) {
		std::vector<std::string> s = split(conf[i]);
		for (size_t y = 0; y < s.size(); y++) {
			std::cout << s[y] << " ";
		}
		std::cout << "___\n";

		if (s[0] == "server" && s[1] == "{") {
			i++;
			continue;
		}
		else if (s[0].find("location") != std::string::npos) {
			for (size_t x = 1; x < s.size() && s[x] != "{"; x++)
				locName += s[x];
		}
		else
			locName = "location";

		std::cout << "HERE1\n";
		size_t a = s.size() - 1;
		size_t b = s[a].size() - 1;
		if (s[a][b] == ';') {
			std::string t = s[a].substr(0, s[a].size() - 1);
			s[a] = t;
		}

		if (s[0].find("listen") != std::string::npos || s[0].find("port") != std::string::npos) {
			std::cout << "HERE3\n";
			serv.port = s[1];//std::atoi(s[1].c_str());
		}
		else if (s[0].find("server_name") != std::string::npos) {
			std::cout << "HERE4\n";
			serv.servName = s[1];
		}
		else if (s[0].find("error_page") != std::string::npos) {
			std::cout << "HERE5\n";
			for (size_t j = 2; j < s.size(); j += 2)
				serv.errPages.insert(std::pair<int, std::string>(atoi(s[j - 1].c_str()), s[j].substr(0, s[j].size() - 2)));
		}
		else if (s[0].find("client_max_body_size") != std::string::npos) {
			std::cout << "HERE6\n";
			serv.maxRequestSize = s[1];
		}
		else if (!s[0].empty()) {
			std::cout << "HERE7\n";
			setLocationLevel(i, serv, conf, s);
		}
		i++;
		std::cout << "HERE8\n";
	}
	std::cout << "HERE9\n";
}

void Config::parseAndSetConfigs() {
	for (size_t i = 0; i < _storedConfigs.size(); i++) {
		//TODO: add error check!
		struct serverLevel nextConf;
		setServerLevel(nextConf, _storedConfigs[i]);
		_allConfigs.push_back(nextConf);
		if (i == 0)
			_config = _allConfigs[0];
	}
}

/* ************************************************************************************** */

// std::map<std::string, std::string> const &Config::getConfig() const {
// 	return _config;
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

void Config::printAllConfigs() {
	for (size_t i = 0; i < _storedConfigs.size(); i++) {
		std::cout << "config[" << i << "]\n";
		for (size_t j = 0; j < _storedConfigs[i].size(); j++) {
			std::cout << _storedConfigs[i][j] << std::endl;
		}
	}
}

// const std::string& Config::getConfigValue(const std::string& key) const {
// 	std::map<std::string, std::string> tmp = getConfig();
// 	std::map<std::string, std::string>::const_iterator it = tmp.find(key);
// 	if (it != tmp.end())
// 		return it->second;
// 	static const std::string empty = "";
// 	return empty;
// }

// void Config::printConfig() {
// 	std::map<std::string, std::string>::iterator it = _config.begin();
// 	while (it != _config.end()) {
// 		std::cout << it->first << " = " << it->second << std::endl;
// 		++it;
// 	}
// }
