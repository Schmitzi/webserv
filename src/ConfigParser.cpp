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
		_ipPortToServers = copy._ipPortToServers;
	}
	return (*this);
}

ConfigParser::~ConfigParser() {
    _ipPortToServers.clear();

	std::vector<serverLevel>::iterator servIt = _allConfigs.begin();
	for (; servIt != _allConfigs.end(); ++servIt) {
		std::map<std::string, locationLevel>::iterator locIt = servIt->locations.begin();
		for (; locIt != servIt->locations.end(); ++locIt)
            locIt->second.methods.clear();
        servIt->locations.clear();
        servIt->port.clear();
        servIt->servName.clear();
        servIt->errPages.clear();
	}
	_allConfigs.clear();

    std::vector<std::vector<std::string> >::iterator it = _storedConfigs.begin();
    for (; it != _storedConfigs.end(); ++it)
        it->clear();
    _storedConfigs.clear();
}

/* ***************************************************************************************** */
// SETTERS

void ConfigParser::storeConfigs() {
	int configNum;

	std::ifstream file(_filepath.c_str());
	if (!file.is_open() || !file.good())
		throw configException("Error: Invalid or empty config file.");
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
}

void ConfigParser::setLocationLevel(size_t &i, std::vector<std::string>& s, serverLevel &serv, std::vector<std::string> &conf) {
	locationLevel loc;
	initLocLevel(s, loc);
	while (i < conf.size()) {
		if (conf[i].find("}") != std::string::npos) break;
		else if (!whiteLine(conf[i])) {
			s = splitIfSemicolon(conf[i]);
			if (s[0] == "root") setRootLoc(loc, s);
			else if (s[0] == "index") setLocIndexFile(loc, s);
			else if (s[0] == "methods") setMethods(loc, s);
			else if (s[0] == "autoindex") setAutoindex(loc, s);
			else if (s[0] == "redirect") setRedirection(loc, s);
			else if (s[0] == "cgi_pass") setCgiProcessorPath(loc, s);
			else if (s[0] == "upload_store") setUploadDirPath(loc, s);
		}
		i++;
	}
	if (conf[i].find("}") == std::string::npos)
		throw configException("Error: no closing bracket found for location.");
	checkMethods(loc);
	serv.locations.insert(std::pair<std::string, locationLevel>(loc.locName, loc));
}

void ConfigParser::setServerLevel(size_t &i, std::vector<std::string> &s, serverLevel &serv, std::vector<std::string> &conf) {
	while (i < conf.size() && conf[i].find("}") == std::string::npos) {
		if (conf[i].find("location ") != std::string::npos) {
			i--;
			return;
		}
		if (!whiteLine(conf[i])) {
			s = splitIfSemicolon(conf[i]);
			if (s[0] == "listen") setPort(s, serv);
			else if (s[0] == "root") setRootServ(serv, s);
			else if (s[0] == "index") setServIndexFile(serv, s);
			else if (s[0] == "server_name") setServName(serv, s);
			else if (s[0] == "error_page") setErrorPages(s, serv);
			else if (s[0] == "client_max_body_size") setMaxRequestSize(serv, s);
		}
		i++;
	}
	if (conf[i].find("}") != std::string::npos) i--;
}

void ConfigParser::setConfigLevels(serverLevel& serv, std::vector<std::string>& conf) {
	bool	bracket;
	size_t	i;

	std::vector<std::string> s;
	bracket = false;
	i = 0;
	while (i < conf.size()) {
		if (!whiteLine(conf[i])) {
			s = split(conf[i]);
			if (s.back() == "{") {
				i++;
				if (foundServer(s)) setServerLevel(i, s, serv, conf);
				else if (foundLocation(s)) setLocationLevel(i, s, serv, conf);
				else throw configException("Error: something invalid in config.");
			}
			else if (s.back() == "}") checkBracket(s, bracket);
		}
		i++;
	}
	if (bracket == false)
		throw configException("Error: No closing bracket found for server.");
	checkConfig(serv);
}

void ConfigParser::setIpPortToServers() {
	for (size_t i = 0; i < _allConfigs.size(); ++i) {
		for (size_t j = 0; j < _allConfigs[i].port.size(); ++j) {
			std::pair<std::string, int> ipPort = _allConfigs[i].port[j];
			std::vector<serverLevel*>& servers = _ipPortToServers[ipPort];
			if (std::find(servers.begin(), servers.end(), &_allConfigs[i]) == servers.end()) {
				servers.push_back(&_allConfigs[i]);
			}
		}
	}
}

void ConfigParser::parseAndSetConfigs() {
	for (size_t i = 0; i < _storedConfigs.size(); i++) {
		serverLevel nextConf;
		setConfigLevels(nextConf, _storedConfigs[i]);
		_allConfigs.push_back(nextConf);
	}
	// printAllConfigs();
	setIpPortToServers();
	// printIpPortToServers();
}

/* *************************************************************************************** */
// GETTERS

std::vector<std::vector<std::string> > ConfigParser::getStoredConfigs() {
	return _storedConfigs;
}

std::vector<serverLevel> ConfigParser::getAllConfigs() {
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

void ConfigParser::printIpPortToServers() const {
	std::map<std::pair<std::string, int>, std::vector<serverLevel*> >::const_iterator it = _ipPortToServers.begin();
	for (; it != _ipPortToServers.end(); ++it) {
		const std::pair<std::string, int>& ipPort = it->first;
		const std::vector<serverLevel*>& servers = it->second;

		std::cout << "IP: " << ipPort.first << ", Port: " << ipPort.second << std::endl;
		std::cout << "  Associated Servers: " << std::endl;

		for (size_t i = 0; i < servers.size(); ++i) {
			std::cout << "    - ServerConfig at address: " << servers[i];
			if (!servers[i]->servName.empty()) {
				std::cout << " (name: " << servers[i]->servName[0] << ")";
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
}