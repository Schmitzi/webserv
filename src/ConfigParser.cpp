#include "../include/ConfigParser.hpp"

ConfigParser::ConfigParser() : _filepath("config/default.conf") {
	storeConfigs();
	parseAndSetConfigs();
}

ConfigParser::ConfigParser(const std::string& filepath) {
	_filepath = filepath;
	std::string s = filepath.substr(strlen(filepath.c_str()) - 5, 5);
	if (s != ".conf") {
		std::cerr << "Invalid config file specified -> using default config file" << std::endl;
		_filepath = "config/default.conf";
	}
	storeConfigs();
	parseAndSetConfigs();
}

ConfigParser::ConfigParser(const ConfigParser& copy) {
	*this = copy;
}

ConfigParser &ConfigParser::operator=(const ConfigParser& copy) {
	if (this != &copy) {
		_filepath = copy._filepath;
		_storedConfigs = copy._storedConfigs;
		_allConfigs = copy._allConfigs;
	}
	return (*this);
}

ConfigParser::~ConfigParser() {}

/* ***************************************************************************************** */
// SETTERS

void ConfigParser::storeConfigs() {
	int configNum;

	std::ifstream file(_filepath.c_str());
	if (!file.is_open() || !file.good()) {
		std::cerr << "Invalid or empty config file." << std::endl;
		return ;
	}
	std::string line;
	configNum = -1;
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

void ConfigParser::setLocationLevel(size_t &i, std::vector<std::string> &s, struct serverLevel &serv, std::vector<std::string> &conf) {
	struct locationLevel loc;

	loc.autoindex = false;
	loc.autoindexFound = false;
	std::string locName;
	for (size_t x = 1; x < s.size() && s[x] != "{"; x++)
		locName += " " + s[x];
	while (i < conf.size()) {
		if (conf[i].find("}") != std::string::npos)
			break ;
		else if (!whiteLine(conf[i])) {
			s = splitIfSemicolon(conf[i]);
			if (s[0] == "root") {
				if (!isValidDir(s[1]))
					throw configException("Error: invalid directory path -> " + s[0] + s[1]);
				loc.rootLoc = s[1];
			}
			else if (s[0] == "index") {
				if (!isValidIndexFile(s[1]))
					throw configException("Error: invalid path -> " + s[0] + s[1]);
				loc.indexFile = s[1];
			}
			else if (s[0] == "methods") {
				for (size_t m = 1; m < s.size(); m++) {
					if (s[m] != "GET" && s[m] != "DELETE" && s[m] != "POST")
						throw configException("Error: Invalid method found: " + s[m]);
					loc.methods.push_back(s[m]);
				}
			}
			else if (s[0] == "autoindex") {
				loc.autoindexFound = true;
				if (s[1] == "on")
					loc.autoindex = true;
			}
			else if (s[0] == "redirect") {
				if (!isValidRedirectPath(s[1]))
					throw configException("Error: invalid path -> " + s[0] + s[1]);
				loc.redirectionHTTP = s[1];
			}
			else if (s[0] == "cgi_pass") {
				if (!isValidDir(s[1]))
					throw configException("Error: invalid directory path -> " + s[0] + s[1]);
				loc.cgiProcessorPath = s[1];
			}
			else if (s[0] == "upload_store") {
				if (!isValidDir(s[1]))
					throw configException("Error: invalid directory path -> " + s[0] + s[1]);
				loc.uploadDirPath = s[1];
			}
		}
		i++;
	}
	if (conf[i].find("}") == std::string::npos)
		throw configException("Error: no closing bracket found.");
	serv.locations.insert(std::pair<std::string, struct locationLevel>(locName, loc));
}

void ConfigParser::setServerLevel(size_t &i, std::vector<std::string> &s, struct serverLevel &serv, std::vector<std::string> &conf) {
	while (i < conf.size() && conf[i].find("}") == std::string::npos) {
		if (conf[i].find("location ") != std::string::npos) {
			i--;
			return ;
		}
		if (!whiteLine(conf[i])) {
			s = splitIfSemicolon(conf[i]);
			if (s[0] == "listen")
				serv.port = std::atoi(s[1].c_str());
			else if (s[0] == "root") {
				if (!isValidDir(s[1]))
					throw configException("Error: invalid directory path -> " + s[0] + s[1]);
				serv.rootServ = s[1];
			}
			else if (s[0] == "index") {
				if (!isValidIndexFile(s[1]))
					throw configException("Error: invalid path -> " + s[0] + s[1]);
				serv.indexFile = s[1];
			}
			else if (s[0] == "server_name") {
				if (!isValidName(s[1]))
					throw configException("Error: invalid " + s[0] + " -> " + s[1]);
				serv.servName = s[1];
			}
			else if (s[0] == "error_page")
				setErrorPages(s, serv);
			else if (s[0] == "client_max_body_size") {
				serv.maxRequestSize = s[1];
				parseClientMaxBodySize(serv);
			}
		}
		i++;
	}
}

void ConfigParser::setConfigLevels(struct serverLevel &serv, std::vector<std::string> &conf) {
	bool	bracket;
	size_t	i;

	std::vector<std::string> s;
	bracket = false;
	i = 0;
	while (i < conf.size() && bracket == false) {
		if (!whiteLine(conf[i])) {
			s = split(conf[i]);
			if (s.back() == "{") {
				i++;
				if (s[0] == "server") {
					if (s[1] != "{")
						throw configException("Error: No opening bracket found for server.");
					setServerLevel(i, s, serv, conf);
				}
				else if (s[0] == "location") {
					if (s.back() != "{")
						throw configException("Error: No opening bracket found for location.");
					setLocationLevel(i, s, serv, conf);
				}
			}
			else if (s[0] == "}")
				bracket = true;
		}
		i++;
	}
	if (bracket == false)
		throw configException("Error: No closing bracket found for server.");
	checkConfig(serv);
}

void ConfigParser::parseAndSetConfigs() {
	struct serverLevel nextConf;
	for (size_t i = 0; i < _storedConfigs.size(); i++) {
		nextConf.port = -1;
		setConfigLevels(nextConf, _storedConfigs[i]);
		_allConfigs.push_back(nextConf);
	}
}

/* *************************************************************************************** */
// GETTERS

std::vector<std::vector<std::string> > ConfigParser::getStoredConfigs() {
	return _storedConfigs;
}

std::vector<struct serverLevel> ConfigParser::getAllConfigs() {
	return _allConfigs;
}

/* ************************************************************************************** */
// EXTRAS

void ConfigParser::printAllConfigs() {
	for (size_t i = 0; i < _storedConfigs.size(); i++) {
		std::cout << "config[" << i << "]\n";
		for (size_t j = 0; j < _storedConfigs[i].size(); j++)
			std::cout << _storedConfigs[i][j] << std::endl;
	}
}
