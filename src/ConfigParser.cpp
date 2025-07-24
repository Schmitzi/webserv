#include "../include/ConfigParser.hpp"
#include "../include/Helper.hpp"
#include "../include/ConfigHelper.hpp"
#include "../include/ConfigValidator.hpp"

serverLevel::serverLevel() : 
		port(),
		rootServ(""),
		indexFile(""),
		servName(),
		errPages(),
		maxRequestSize(""),
		requestLimit(0),
		locations() {}

locationLevel::locationLevel() :
		locName(""),
		rootLoc(""),
		indexFile(""),
		methods(),
		autoindex(false),
		redirectionHTTP(std::pair<int, std::string>(0, "")),
		hasRedirect(false),
		cgiProcessorPath(""),
		uploadDirPath(""),
		isRegex(false) {}

serverLevel::~serverLevel() {}

locationLevel::~locationLevel() {}

ConfigParser::ConfigParser() {
	_filepath = "";
	std::vector<std::string> temp;
	temp.push_back("");
	_storedConfigs.push_back(temp);
	serverLevel serv = serverLevel();
	_allConfigs.push_back(serv);
	std::pair<std::pair<std::string, int>, bool> second;
	second.first = std::pair<std::string, int>("", -1);
	second.second = false;	
	std::vector<serverLevel> third;
	third.push_back(serv);
}

ConfigParser::ConfigParser(const std::string& filepath) {
	_filepath = filepath;
	std::string s = filepath.substr(strlen(filepath.c_str()) - 5, 5);
	if (s != ".conf")
		throw configException("Error: Invalid config file specified.");
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

ConfigParser::~ConfigParser() {
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
	if (!file.good())
		throw configException("Error: Invalid or empty config file.");
	std::string line;
	configNum = -1;
	while (std::getline(file, line)) {
		line = skipComments(line);
		if (!line.empty() && iFind(line, "server {") != std::string::npos) {
			nextConfig:
			configNum++;
			_storedConfigs.resize(configNum + 1);
			_storedConfigs[configNum].push_back(line);
			while (std::getline(file, line) && iFind(line, "server {") == std::string::npos) {
				line = skipComments(line);
				if (!line.empty())
					_storedConfigs[configNum].push_back(line);
			}
			line = skipComments(line);
			if (!line.empty() && iFind(line, "server {") != std::string::npos)
				goto nextConfig;
		}
	}
}

void ConfigParser::setLocationLevel(size_t &i, std::vector<std::string>& s, serverLevel &serv, std::vector<std::string> &conf) {
	locationLevel loc = locationLevel();
	initLocLevel(s, loc);
	while (i < conf.size()) {
		if (conf[i].find("}") != std::string::npos) break;
		else if (!whiteLine(conf[i])) {
			s = splitIfSemicolon(conf[i]);
			if (iEqual(s[0], "root")) setRootLoc(loc, s);
			else if (iEqual(s[0], "index")) setLocIndexFile(loc, s);
			else if (iEqual(s[0], "limit_except")) setMethods(loc, s);
			else if (iEqual(s[0], "autoindex")) setAutoindex(loc, s);
			else if (iEqual(s[0], "return")) setRedirection(loc, s);
			else if (iEqual(s[0], "cgi_pass")) setCgiProcessorPath(loc, s);
			else if (iEqual(s[0], "upload_store")) setUploadDirPath(loc, s);
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
		if (iFind(conf[i], "location ") != std::string::npos) {
			i--;
			return;
		}
		if (!whiteLine(conf[i])) {
			s = splitIfSemicolon(conf[i]);
			if (iEqual(s[0], "listen")) setPort(s, serv);
			else if (iEqual(s[0], "server_name")) setServName(serv, s);
			else if (iEqual(s[0], "root")) setRootServ(serv, s);
			else if (iEqual(s[0], "index")) setServIndexFile(serv, s);
			else if (iEqual(s[0], "error_page")) setErrorPages(s, serv);
			else if (iEqual(s[0], "client_max_body_size")) setMaxRequestSize(serv, s);
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

void ConfigParser::parseAndSetConfigs() {
	std::set<std::string> usedCombinations;
	for (size_t i = 0; i < _storedConfigs.size(); i++) {
		serverLevel nextConf = serverLevel();
		setConfigLevels(nextConf, _storedConfigs[i]);
		bool validServer = false;

		for (size_t j = 0; j < nextConf.port.size(); j++) {
			for (size_t k = 0; k < nextConf.servName.size(); k++) {
				std::string combination = nextConf.port[j].first.first + ":" + tostring(nextConf.port[j].first.second) + ":" + nextConf.servName[k];
				if (usedCombinations.find(combination) == usedCombinations.end()) {
					usedCombinations.insert(combination);
					validServer = true;
				} else throw configException("Error: Duplicate server configuration found for " + combination);
			}
		}
		if (validServer)
			_allConfigs.push_back(nextConf);
	}
}

/* *************************************************************************************** */
// GETTERS

std::vector<serverLevel> ConfigParser::getAllConfigs() {
	return _allConfigs;
}

int ConfigParser::getPort(serverLevel& conf) {
	for (size_t i = 0; i < conf.port.size(); i++) {
		std::pair<std::pair<std::string, int>, bool> ipPort = conf.port[i];
		if (ipPort.second == true)
			return ipPort.first.second;
	}
	return conf.port[0].first.second;
}

std::pair<std::pair<std::string, int>, bool> ConfigParser::getDefaultPortPair(serverLevel& conf) {
	for (size_t i = 0; i < conf.port.size(); i++) {
		std::pair<std::pair<std::string, int>, bool> ipPort = conf.port[i];
		if (ipPort.second == true)
			return ipPort;
	}
	return conf.port[0];
}

serverLevel& ConfigParser::getConfigByIndex(size_t nbr) {
	if (nbr >= _allConfigs.size())
		throw configException("Error: Invalid config index specified.");
	return _allConfigs[nbr];
}

/* ************************************************************************************** */
// EXTRAS

void ConfigParser::printAllConfigs() {
	for (size_t i = 0; i < _storedConfigs.size(); i++) {
		std::cout << "___config[" << i << "]___\n";
		for (size_t j = 0; j < _storedConfigs[i].size(); j++) {
			if (!whiteLine(_storedConfigs[i][j]))
				std::cout << _storedConfigs[i][j] << std::endl;
		}
		std::cout << "____________________________________" << std::endl << std::endl;
	}
}
