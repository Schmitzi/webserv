#include "../include/ConfigParser.hpp"

ConfigParser::ConfigParser() : _filepath("config/default.conf") {
	storeConfigs();
	parseAndSetConfigs();
}

ConfigParser::ConfigParser(std::string const &filepath) {
	_filepath = filepath;
	std::string s = filepath.substr(strlen(filepath.c_str()) - 5, 5);
	if (s != ".conf") {
		std::cerr << "Invalid config file specified -> using default config file" << std::endl;
		_filepath = "config/default.conf";
	}
	storeConfigs();
	parseAndSetConfigs();
}

ConfigParser::ConfigParser(const ConfigParser& other) {
	*this = other;
}

ConfigParser &ConfigParser::operator=(const ConfigParser& other) {
	if (this != &other) {
		_filepath = other._filepath;
		_storedConfigs = other._storedConfigs;
		_allConfigs = other._allConfigs;
	}
	return *this;
}

ConfigParser::~ConfigParser() {}

/* ************************************************************************************** */
//EXTRAS

void ConfigParser::printAllConfigs() {
	for (size_t i = 0; i < _storedConfigs.size(); i++) {
		std::cout << "config[" << i << "]\n";
		for (size_t j = 0; j < _storedConfigs[i].size(); j++) {
			std::cout << _storedConfigs[i][j] << std::endl;
		}
	}
}

bool ConfigParser::whiteLine(std::string& line) {
	if (line.empty())
		return true;
	for (size_t i = 0; i < line.size(); i++) {
		if (!isspace(line[i]))
			return false;
	}
	return true;
}

bool ConfigParser::checkSemicolon(std::string& line) {
	if (line[line.size() - 1] == ';')
		return true;
	return false;
}

std::string ConfigParser::skipComments(std::string& s) {
	std::string ret;
	size_t x;
	x = s.find("#");
	if (x != std::string::npos) {
		ret = s.substr(0, x);
		return ret;
	}
	x = s.find("//");
	if (x != std::string::npos) {
		ret = s.substr(0, x);
		return ret;
	}
	ret = s;
	return ret;
}

bool ConfigParser::isValidDir(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
        return false;
    if (!S_ISDIR(info.st_mode))
        return false;
    if (access(path.c_str(), R_OK | X_OK) != 0)
        return false;
    return true;
}

/* ***************************************************************************************** */
//SETTERS

void ConfigParser::storeConfigs() {
	std::ifstream file(_filepath.c_str());
	if (!file.good()) {
		std::cerr << "Invalid or empty config file." << std::endl;
		return;
	}
	std::string line;
	int configNum = -1;
	while (std::getline(file, line)) {
		line = skipComments(line);
		if (!line.empty() && line.find("server {") != std::string::npos) {
			nextConfig:
			configNum++;
			_storedConfigs.resize(configNum + 1);
			_storedConfigs[configNum].push_back(line);
			while (std::getline(file, line) && line.find("server {") == std::string::npos) {
				line = skipComments(line);
				if (!line.empty())
					_storedConfigs[configNum].push_back(line);
			}
			line = skipComments(line);
			if (!line.empty() && line.find("server {") != std::string::npos)
				goto nextConfig;
		}
	}
	// printAllConfigs();
}

void ConfigParser::setLocationLevel(size_t& i, std::vector<std::string>& s, struct serverLevel& serv, std::vector<std::string>& conf) {
	struct locationLevel loc;
	loc.autoindex = false;
	std::string locName;
	for (size_t x = 1; x < s.size() && s[x] != "{"; x++)
		locName += " " + s[x];
	while (i < conf.size() && conf[i].find("}") == std::string::npos) {
		if (!whiteLine(conf[i])) {
			if (!checkSemicolon(conf[i]))
				throw std::runtime_error("Error: no semicolon found in config file (locationLevel)");
			conf[i] = conf[i].substr(0, conf[i].size() - 1);
			s = split(conf[i]);
			if (s[0] == "root") {
				loc.docRootDir = s[1];
				if (!isValidDir(loc.docRootDir))
					std::cerr << "Error: invalid directory path in config file (docRootDir) -> " << loc.docRootDir << std::endl;
			}
			else if (s[0] == "index")
				loc.indexFile = s[1];
			else if (s[0] == "methods") {
				for (size_t m = 1; m < s.size(); m++)
					loc.methods.push_back(s[m]);
			}
			else if (s[0] == "autoindex")
				loc.autoindex = true;
			else if (s[0] == "redirect")
				loc.redirectionHTTP = s[1];
			else if (s[0] == "cgi_pass") {
				loc.cgiProcessorPath = s[1];
				if (!isValidDir(loc.cgiProcessorPath))
					std::cerr << "Error: invalid directory path in config file (cgiProcessorPath) -> " << loc.cgiProcessorPath << std::endl;
			}
			else if (s[0] == "upload_store") {
				loc.uploadDirPath = s[1];
				if (!isValidDir(loc.uploadDirPath))
					std::cerr << "Error: invalid directory path in config file (uploadDirPath) -> " << loc.uploadDirPath << std::endl;
			}
		}
		i++;
	}
	serv.locations.insert(std::pair<std::string, struct locationLevel>(locName, loc));
}

void ConfigParser::setServerLevel(size_t& i, std::vector<std::string>& s, struct serverLevel& serv, std::vector<std::string>& conf) {
	while (i < conf.size()) {
		if (conf[i].find("location ") != std::string::npos) {
			i--;
			return;
		}
		if (!whiteLine(conf[i])) {
			if (!checkSemicolon(conf[i]))
				throw std::runtime_error("Error: no semicolon found in config file (serverLevel)");
			conf[i] = conf[i].substr(0, conf[i].size() - 1);
			s = split(conf[i]);
			if (s[0] == "listen")
				serv.port = std::atoi(s[1].c_str());
			else if (s[0] == "server_name")
				serv.servName = s[1];
			else if (s[0] == "error_page") {
				for (size_t j = 2; j < s.size(); j += 2)
					serv.errPages.insert(std::pair<int, std::string>(atoi(s[j - 1].c_str()), s[j].substr(0, s[j].size())));
			}
			else if (s[0] == "client_max_body_size")
				serv.maxRequestSize = s[1];
		}
		i++;
	}
}

void ConfigParser::setConfigLevels(struct serverLevel& serv, std::vector<std::string>& conf) {
	std::vector<std::string> s;
	size_t i = 0;
	while (i < conf.size()) {
		if (!whiteLine(conf[i])) {
			s = split(conf[i]);
			if (s[s.size() - 1] == "{") {
				i++;
				if (s[0] == "server" && s[1] == "{")
					setServerLevel(i, s, serv, conf);
				else if (s[0] == "location" && s[s.size() - 1] == "{")
					setLocationLevel(i, s, serv, conf);
			}
		}
		i++;
	}
}

void ConfigParser::parseAndSetConfigs() {
	for (size_t i = 0; i < _storedConfigs.size(); i++) {
		struct serverLevel nextConf;
		nextConf.port = -1;
		setConfigLevels(nextConf, _storedConfigs[i]);
		_allConfigs.push_back(nextConf);
	}
}

/* *************************************************************************************** */
//GETTERS

std::vector<std::vector<std::string> > ConfigParser::getStoredConfigs() {
	return _storedConfigs;
}

std::vector<struct serverLevel> ConfigParser::getAllConfigs() {
	return _allConfigs;
}
