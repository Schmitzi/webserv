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

bool ConfigParser::onlyDigits(const std::string& s) {
	for (size_t i = 0; i < s.size(); i++) {
		if (!isdigit(s[i]))
			return false;
	}
	return true;
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
	return line[line.size() - 1] == ';';
}

std::string ConfigParser::skipComments(std::string& s) {
	std::string ret = s;
	size_t x;
	x = s.find("#");
	if (x != std::string::npos)
		ret = s.substr(0, x);
	x = s.find("//");
	if (x != std::string::npos)
		ret = s.substr(0, x);
	return ret;
}

bool ConfigParser::isValidPath(const std::string& path) {
	struct stat info;
	if (stat(path.c_str(), &info) != 0 || S_ISDIR(info.st_mode) || access(path.c_str(), R_OK) != 0)
		return false;
	return true;
}

bool ConfigParser::isValidRedirectPath(const std::string& path) {
	if (path.empty() || (path[0] != '/' && path.find("http") != 0))
		return false;
	return true;
}

bool ConfigParser::isValidDir(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0 || !S_ISDIR(info.st_mode) || access(path.c_str(), R_OK | X_OK) != 0)
        return false;
    return true;
}

bool ConfigParser::isValidName(const std::string& name) {//TODO: check how server_names work!!
	if (name.empty())
		return true;
	if (name.size() > 253)
		return false;
	std::string::const_iterator it = name.begin();
	std::string label;
	for (; it != name.end(); ++it) {
		char c = *it;
		if (c == '.') {
			if (label.empty() || label[0] == '-' || label[label.size() - 1] == '-')
				return false;
			label.clear();
		}
		else if (isalnum(c) || c == '-')
			label += c;
		else
			return false;
	}
	if (label.empty() || label[0] == '-' || label[label.size() - 1] == '-')
		return false;
	return true;
}

bool ConfigParser::isValidIndexFile(const std::string& indexFile) {
	if (indexFile.empty() || indexFile.find('/') != std::string::npos || indexFile.find("..") != std::string::npos)
		return false;
	// std::string s = "local/" + indexFile;//TODO: how should this be checked?
	// std::ifstream file(s.c_str());
	// if (!file.is_open() || !file.good())
	// 	throw configException("Error: Invalid indexFile -> " + indexFile);
	std::string::size_type dot = indexFile.rfind('.');
	if (dot == std::string::npos)
		return false;
	std::string ext = indexFile.substr(dot);
	return ext == ".html";//TODO: maybe add more extension checks?
}


void ConfigParser::parseClientMaxBodySize(struct serverLevel& serv) {
	size_t multiplier = 1;
	char unit = serv.maxRequestSize[serv.maxRequestSize.size() - 1];
	std::string numberPart = serv.maxRequestSize;
	if (!std::isdigit(unit)) {
		switch (std::tolower(unit)) {
			case 'k': multiplier = 1024; break;
			case 'm': multiplier = 1024 * 1024; break;
			case 'g': multiplier = 1024 * 1024 * 1024; break;
			default:
				throw configException("Invalid size unit for client_max_body_size");
		}
		numberPart = serv.maxRequestSize.substr(0, serv.maxRequestSize.size() - 1);
	}
	size_t num = std::strtoul(numberPart.c_str(), NULL, 10);
	serv.requestLimit = num * multiplier;
}

void ConfigParser::checkRoot(struct serverLevel& serv) {
	if (serv.rootServ.empty()) {
		std::map<std::string, struct locationLevel>::iterator it = serv.locations.begin();
		while (it != serv.locations.end()) {
			if (it->second.rootLoc.empty())
				throw configException("Error: No root for server and locations specified.\n-> Server doesn’t know where to serve files from");
			++it;
		}
	}
}

void ConfigParser::checkIndex(struct serverLevel& serv) {
	if (serv.indexFile.empty()) {
		std::map<std::string, struct locationLevel>::iterator it = serv.locations.begin();
		while (it != serv.locations.end()) {
			if (it->second.indexFile.empty())
				throw configException("Error: No default index for server and locations specified.\n-> Requests to / may return 403 or 404");
			++it;
		}
	}
}

void ConfigParser::checkConfig(struct serverLevel& serv) {
	if (serv.port < 0)
		throw configException("Error: No port specified in config.\n-> server won't bind to any port");
	// if (serv.servName.empty())
	// 	throw configException("Error: No server_name specified in config.\n-> Virtual hosting may break / default fallback");
	checkRoot(serv);
	checkIndex(serv);
	//TODO: does every serverLevel need at least one location? (->recommended, but not needed, but maybe needed for out server??)
	// std::map<std::string, struct locationLevel>::iterator it = serv.locations.begin();
	// if (it == serv.locations.end())
	// 	throw configException("Error: no locations specified.\n-> All URL requests fall back to root config");
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
	loc.autoindexFound = false;
	std::string locName;
	for (size_t x = 1; x < s.size() && s[x] != "{"; x++)
		locName += " " + s[x];
	while (i < conf.size()) {
		if (conf[i].find("}") != std::string::npos)
			break;
		else if (!whiteLine(conf[i])) {
			if (!checkSemicolon(conf[i]))
				throw configException("Error: no semicolon found (locationLevel)");
			conf[i] = conf[i].substr(0, conf[i].size() - 1);
			s = split(conf[i]);
			if (s[0] == "root") {
				if (!isValidDir(s[1]))
					throw configException("Error: invalid directory path (rootLoc) -> " + s[1]);
				loc.rootLoc = s[1];
			}
			else if (s[0] == "index") {
				if (!isValidIndexFile(s[1]))
					throw configException("Error: invalid path (index) -> " + s[1]);
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
					throw configException("Error: invalid path (redirect) -> " + s[1]);
				loc.redirectionHTTP = s[1];
			}
			else if (s[0] == "cgi_pass") {
				if (!isValidDir(s[1]))
					throw configException("Error: invalid directory path (cgiProcessorPath) -> " + s[1]);
				loc.cgiProcessorPath = s[1];
			}
			else if (s[0] == "upload_store") {
				if (!isValidDir(s[1]))
					throw configException("Error: invalid directory path (uploadDirPath) -> " + s[1]);
				loc.uploadDirPath = s[1];
			}
		}
		i++;
	}
	if (conf[i].find("}") == std::string::npos)
		throw configException("Error: no closing bracket found.");
	serv.locations.insert(std::pair<std::string, struct locationLevel>(locName, loc));
}

void ConfigParser::setServerLevel(size_t& i, std::vector<std::string>& s, struct serverLevel& serv, std::vector<std::string>& conf) {
	while (i < conf.size() && conf[i].find("}") == std::string::npos) {
		if (conf[i].find("location ") != std::string::npos) {
			i--;
			return;
		}
		if (!whiteLine(conf[i])) {
			if (!checkSemicolon(conf[i]))//TODO: extract and make new function for this?
				throw configException("Error: no semicolon found (serverLevel)");
			conf[i] = conf[i].substr(0, conf[i].size() - 1);
			s = split(conf[i]);
			if (s[0] == "listen")
				serv.port = std::atoi(s[1].c_str());
			else if (s[0] == "root") {
				if (!isValidDir(s[1]))
					throw configException("Error: invalid directory path (rootServ) -> " + s[1]);
				serv.rootServ = s[1];
			}
			else if (s[0] == "index") {
				if (!isValidIndexFile(s[1]))
					throw configException("Error: invalid path (index) -> " + s[1]);
				serv.indexFile = s[1];
			}
			else if (s[0] == "server_name") {
				if (!isValidName(s[1]))
					throw configException("Error: invalid server_name -> " + s[1]);
				serv.servName = s[1];
			}
			else if (s[0] == "error_page") {
				std::string site;
				int err;
				std::vector<int> errCodes;
				bool waitingForPath = true;
				for (size_t j = 1; j < s.size(); j++) {
					if (onlyDigits(s[j])) {
						err = atoi(s[j].c_str());
						if (err < 400 || err > 599)
							throw configException("Error: Invalid error status code.");
						errCodes.push_back(err);
						waitingForPath = true;
					}
					else if (waitingForPath == true && !onlyDigits(s[j]) && !errCodes.empty()) {
						site = s[j].substr(0, s[j].size());
						// if (!isValidPath(site))
						// 	throw configException("Error: Invalid path (error_page) -> " + site);
						serv.errPages.insert(std::pair<std::vector<int>, std::string>(errCodes, site));
						errCodes.clear();
						waitingForPath = false;
					}
					else
						throw configException("Error: invalid error pages.");
				}
				if (waitingForPath == true)
					throw configException("Error: found error status code without path.");
			}
			else if (s[0] == "client_max_body_size") {
				serv.maxRequestSize = s[1];
				parseClientMaxBodySize(serv);
			}
		}
		i++;
	}
}

void ConfigParser::setConfigLevels(struct serverLevel& serv, std::vector<std::string>& conf) {
	std::vector<std::string> s;
	bool bracket = false;
	size_t i = 0;
	while (i < conf.size() && bracket == false) {
		if (!whiteLine(conf[i])) {
			s = split(conf[i]);
			if (s[s.size() - 1] == "{") {
				i++;
				if (s[0] == "server") {
					if (s[1] != "{")
						throw configException("Error: No opening bracket found for server.");
					setServerLevel(i, s, serv, conf);
				}
				else if (s[0] == "location") {
					if (s[s.size() - 1] != "{")
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
