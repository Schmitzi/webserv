#include "../include/ConfigParser.hpp"

ConfigParser::ConfigParser() {}

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
			else if (s[0] == "limit_except") setMethods(loc, s);
			else if (s[0] == "autoindex") setAutoindex(loc, s);
			else if (s[0] == "return") setRedirection(loc, s);
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
			std::pair<std::pair<std::string, int>, bool> ipPort = _allConfigs[i].port[j];
			int port = ipPort.first.second;
			IPPortToServersMap::iterator it = _ipPortToServers.begin();
			while (it != _ipPortToServers.end() && it->first.first.second != port) ++it;
			if (it == _ipPortToServers.end()) {
				std::vector<serverLevel*> servers;
				_ipPortToServers.insert(std::pair<std::pair<std::pair<std::string, int>, bool>, std::vector<serverLevel*> >(ipPort, servers));
			}
			_ipPortToServers[ipPort].push_back(&_allConfigs[i]);
		}
	}
}

// void ConfigParser::parseAndSetConfigs() {
//     std::map<std::pair<std::string, int>, bool> usedIpPorts;
    
//     for (size_t i = 0; i < _storedConfigs.size(); i++) {
//         serverLevel nextConf;
//         setConfigLevels(nextConf, _storedConfigs[i]);
        
//         // Check for duplicate IP:port combinations
//         for (size_t j = 0; j < nextConf.port.size(); j++) {
//             std::pair<std::pair<std::string, int>, bool> ipPort = nextConf.port[j];
// 			nextConf.servName
            
//             if (usedIpPorts.find(ipPort.first) != usedIpPorts.end()) {
//                 std::cerr << RED << "Warning: IP:port combination " << ipPort.first.first << ":" << ipPort.first.second 
//                           << " is already in use by another server. Ignoring duplicate." << RESET << std::endl;
                
//                 // Remove this duplicate from the server's port list
//                 nextConf.port.erase(nextConf.port.begin() + j);
//                 j--;
//             } else {
//                 usedIpPorts[ipPort.first] = true;
//             }
//         }
        
//         // Only add server if it has at least one valid port
//         if (!nextConf.port.empty()) {
//             _allConfigs.push_back(nextConf);
//         } else {
//             std::cerr << "Warning: Server skipped because it has no valid ports." << std::endl;
//         }
//     }
//     //printAllConfigs();
//     setIpPortToServers();
// }

void ConfigParser::parseAndSetConfigs() {
    std::set<std::string> usedCombinations; // "ip:port:servername"
    
    for (size_t i = 0; i < _storedConfigs.size(); i++) {
        serverLevel nextConf;
        setConfigLevels(nextConf, _storedConfigs[i]);
        
        bool validServer = false;
        for (size_t j = 0; j < nextConf.port.size(); j++) {
            for (size_t k = 0; k < nextConf.servName.size(); k++) {
                std::string combination = nextConf.port[j].first.first + ":" + 
                                        tostring(nextConf.port[j].first.second) + ":" + 
                                        nextConf.servName[k];
                
                if (usedCombinations.find(combination) == usedCombinations.end()) {
                    usedCombinations.insert(combination);
                    validServer = true;
                } else {
					throw configException("Error: Duplicate server configuration found for " + combination);
                }
            }
        }
        
        if (validServer) {
            _allConfigs.push_back(nextConf);
        }
    }
    // printAllConfigs();
    setIpPortToServers();
	// printIpPortToServers();
}

/* *************************************************************************************** */
// GETTERS

std::vector<serverLevel> ConfigParser::getAllConfigs() {
	return _allConfigs;
}

IPPortToServersMap ConfigParser::getIpPortToServers() {
	return _ipPortToServers;
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

serverLevel& ConfigParser::getConfigByIndex(size_t nbr) {//get a config by index
	if (nbr >= _allConfigs.size())
		throw configException("Error: Invalid config index specified.");
	return _allConfigs[nbr];
}

serverLevel& ConfigParser::getConfigByIpPortPair(const std::pair<std::pair<std::string, int>, bool>& ipPort) {//get a config by ip:port pair
	IPPortToServersMap::iterator it = _ipPortToServers.find(ipPort);
	if (it == _ipPortToServers.end() || it->second.empty())
		throw configException("Error: No server found for the specified IP:port pair.");
	return *(it->second[0]);
}

serverLevel& ConfigParser::getConfigByServerName(const std::string& servName) {//get a config by server name
	for (size_t i = 0; i < _allConfigs.size(); i++) {
		for (size_t j = 0; j < _allConfigs[i].servName.size(); j++) {
			if (_allConfigs[i].servName[j] == servName)
				return _allConfigs[i];
		}
	}
	throw configException("Error: No server found with the specified server name.");
}

serverLevel& ConfigParser::getConfigByServerNameIpPortPair(const std::string& servName, const std::pair<std::string, int>& ipPort) {//get a config by server name and ip:port pair
	IPPortToServersMap::iterator it = _ipPortToServers.find(std::make_pair(ipPort, false));
	if (it == _ipPortToServers.end() || it->second.empty())
		throw configException("Error: No server found for the specified server name and IP:port pair.");
	
	for (size_t i = 0; i < it->second.size(); i++) {
		for (size_t j = 0; j < it->second[i]->servName.size(); j++) {
			if (it->second[i]->servName[j] == servName)
				return *(it->second[i]);
		}
	}
	throw configException("Error: No server found with the specified server name and IP:port pair.");
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

void ConfigParser::printIpPortToServers() {
	IPPortToServersMap::iterator it = _ipPortToServers.begin();
	std::cout << std::endl << "___IP:Port -> Servers___" << std::endl;
	for (; it != _ipPortToServers.end(); ++it) {
		std::pair<std::pair<std::string, int>, bool> ipPort = it->first;
		std::vector<serverLevel*>& servers = it->second;

		std::cout << "IP: " << it->first.first.first << ", Port: " << it->first.first.second << std::endl;
		if (it->first.second == true)
			std::cout << "Default Server: Yes" << std::endl;
		std::cout << "  Associated Servers: " << std::endl;

		for (size_t i = 0; i < servers.size(); ++i)
			std::cout << "    - server_name: " << servers[i]->servName[0] << std::endl;
		std::cout << std::endl;
	}
}

void ConfigParser::printConfig(serverLevel& conf) {//only temporary, for debugging
	std::cout << "___config___" << std::endl
	<< "server {" << std::endl;
	if (!conf.rootServ.empty())
		std::cout << "\troot: " << conf.rootServ << std::endl;
	if (!conf.indexFile.empty())
		std::cout << "\tindex: " << conf.indexFile << std::endl;
	if (!conf.port.empty()) {
		std::cout << "\tport:" << std::endl;
		for (size_t i = 0; i < conf.port.size(); i++) {
			std::cout << "\t\t";
			std::pair<std::pair<std::string, int>, bool> ipPort = conf.port[i];
			if (conf.port[i].first.first != "0.0.0.0")
				std::cout << conf.port[i].first.first << " ";
			std::cout << conf.port[i].first.first << std::endl;
			if (conf.port[i].second == true)
				std::cout << "\t\tdefault_server" << std::endl;
		}
	}
	if (!conf.servName.empty()) {
		for (size_t i = 0; i < conf.servName.size(); i++) {
			if (i == 0)
				std::cout << "\tserver_name:";
			std::cout << " " << conf.servName[i];
		}
		std::cout << std::endl;
	}
	std::map<std::vector<int>, std::string>::iterator it = conf.errPages.begin();
	if (it != conf.errPages.end()) {
		std::cout << "\terror_page:" << std::endl;
		while (it != conf.errPages.end()) {
			std::cout << "\t\t";
			for (size_t i = 0; i < it->first.size(); i++)
				std::cout << it->first[i] << " ";
			std::cout << it->second << std::endl;
			++it;
		}
	}
	if (!conf.maxRequestSize.empty())
		std::cout << "\tclient_max_body_size: " << conf.maxRequestSize << std::endl << std::endl;
	std::map<std::string, locationLevel>::iterator its = conf.locations.begin();
	while (its != conf.locations.end()) {
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
		if (!its->second.redirectionHTTP.second.empty())
			std::cout << "\t\treturn: " << its->second.redirectionHTTP.first << " " << its->second.redirectionHTTP.second << std::endl;
		if (!its->second.cgiProcessorPath.empty())
			std::cout << "\t\tcgi_pass: " << its->second.cgiProcessorPath << std::endl;
		if (!its->second.uploadDirPath.empty())
			std::cout << "\t\tupload_store: " << its->second.uploadDirPath << std::endl;
		std::cout << "\t}" << std::endl << std::endl;
		++its;
	}
	std::cout << "}" << std::endl;
}