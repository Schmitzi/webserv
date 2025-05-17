#include "../include/ConfigHelper.hpp"

bool onlyDigits(const std::string &s) {
	for (size_t i = 0; i < s.size(); i++) {
		if (!isdigit(s[i]))
			return (false);
	}
	return (true);
}

bool whiteLine(std::string &line) {
	if (line.empty())
		return (true);
	for (size_t i = 0; i < line.size(); i++) {
		if (!isspace(line[i]))
			return (false);
	}
	return (true);
}

bool checkSemicolon(std::string &line) {
	return line[line.size() - 1] == ';';
}

std::string skipComments(std::string &s) {
	size_t	x;

	std::string ret = s;
	x = s.find("#");
	if (x != std::string::npos)
	ret = s.substr(0, x);
	x = s.find("//");
	if (s.find("http") == 0 && x != std::string::npos)
	ret = s.substr(0, x);
	return (ret);
}

std::vector<std::string> splitIfSemicolon(std::string &configLine) {
	std::vector<std::string> s;
	if (!checkSemicolon(configLine)) // && configLine.find("http") == 0)
		throw configException("Error: missing semicolon in config");
	configLine = configLine.substr(0, configLine.size() - 1);
	s = split(configLine);
	return s;
}

void setPort(std::vector<std::string>& s, serverLevel& serv) {
	std::string ip = "0.0.0.0";
	int port = 8080;
	bool isDefault = false;

	size_t colon = s[1].find(':');
	size_t defPort = s[1].find("default_server");
	if (colon != std::string::npos) {
		std::string tmp = s[1].substr(0, colon);
		if (tmp != "localhost")
        	ip = s[1].substr(0, colon);
		if (defPort != std::string::npos) {
			isDefault = true;
        	port = std::atoi(s[1].substr(colon + 1, defPort).c_str());
		}
		else
			port = std::atoi(s[1].substr(colon + 1).c_str());
    } else {
		if (defPort != std::string::npos) {
			isDefault = true;
			port = std::atoi(s[1].substr(defPort).c_str());
		}
        else
			port = std::atoi(s[1].c_str());
	}
	if (port < 0)
		port = 8080;
	serv.port.push_back(std::pair<std::pair<std::string, int>, bool>(std::pair<std::string, int>(ip, port), isDefault));
}

void setErrorPages(std::vector<std::string>& s, serverLevel &serv) {
	std::string site;
	std::vector<int> errCodes;
	bool waitingForPath = true;
	int err;

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
			// site = getAbsPath(site);
			if (!isValidPath(site))
				throw configException("Error: Invalid path (error_page) -> " + site);
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

/* ____________________________set Config Levels____________________________ */

bool foundServer(std::vector<std::string>& s) {
	if (s[0] == "server") {
		if (s.size() != 2)
			throw configException("Error: invalid server declaration.");
		if (s.back() != "{")
			throw configException("Error: No opening bracket found for server.");
		return true;
	}
	return false;
}

bool foundLocation(std::vector<std::string>& s) {
	if (s[0] == "location") {
		if (s.back() != "{")
			throw configException("Error: No opening bracket found for location.");
		return true;
	}
	return false;
}

void checkBracket(std::vector<std::string>& s, bool& bracket) {
	if (s.size() != 1)
		throw configException("Error: invalid closing bracket in config.");
	if (bracket == true)
		throw configException("Error: Too many closing brackets found.");
	bracket = true;
}

/* ____________________________set Location Level____________________________ */

void setRootLoc(locationLevel& loc, std::vector<std::string>& s) {
	std::string path = getAbsPath(s[1]);
	if (!path.empty() && !isValidDir(path))
		throw configException("Error: invalid directory path for " + s[0] + " -> " + s[1]);
	loc.rootLoc = path;
}

void setLocIndexFile(locationLevel& loc, std::vector<std::string>& s) {
	std::string path = getAbsPath(s[1]);
	if (!path.empty() && !isValidIndexFile(s[1]))
		throw configException("Error: invalid path for " + s[0] + " -> " + s[1]);
	loc.indexFile = path;
}

void setMethods(locationLevel& loc, std::vector<std::string>& s) {
	for (size_t i = 1; i < s.size(); i++) {
		if (s[i] == "GET" || s[i] == "POST" || s[i] == "DELETE")
			loc.methods.push_back(s[i]);
		else
			throw configException("Error: invalid method -> " + s[i]);
	}
}

void setAutoindex(locationLevel& loc, std::vector<std::string>& s) {
	loc.autoindexFound = true;
	if (s[1] == "on")
		loc.autoindex = true;
	else if (s[1] == "off")
		loc.autoindex = false;
	else
		throw configException("Error: invalid autoindex value -> " + s[1]);
}

void setRedirection(locationLevel& loc, std::vector<std::string>& s) {
	if (!s[1].empty() && !isValidRedirectPath(s[1]))
		throw configException("Error: invalid path for " + s[0] + " -> " + s[1]);
	loc.redirectionHTTP = s[1];
}

void setCgiProcessorPath(locationLevel& loc, std::vector<std::string>& s) {
	std::string path = getAbsPath(s[1]);
	if (!path.empty() && !isValidDir(path))
		throw configException("Error: invalid directory path for " + s[0] + " -> " + s[1]);
	loc.cgiProcessorPath = path;
}

void setUploadDirPath(locationLevel& loc, std::vector<std::string>& s) {
	std::string path = getAbsPath(s[1]);
	if (!path.empty() && !isValidDir(path))
		throw configException("Error: invalid directory path for " + s[0] + " -> " + s[1]);
	loc.uploadDirPath = path;
}

/* ____________________________set Server Level____________________________ */

void setRootServ(serverLevel& serv, std::vector<std::string>& s) {
	if (!s[1].empty() && !isValidDir(s[1]))
		throw configException("Error: invalid directory path for " + s[0] + " -> " + s[1]);
	serv.rootServ = s[1];
}

void setServIndexFile(serverLevel& serv, std::vector<std::string>& s) {
	if (!s[1].empty() && !isValidIndexFile(s[1]))
		throw configException("Error: invalid path for " + s[0] + " -> " + s[1]);
	serv.indexFile = s[1];
}

void setServName(serverLevel& serv, std::vector<std::string>& s) {
	if (!isValidName(s[1]))
		serv.servName[0] = "";	
	else {
		for (size_t j = 1; j < s.size(); j++)
			serv.servName.push_back(s[j]);
	}
}

void setMaxRequestSize(serverLevel& serv, std::vector<std::string>& s) {
	if (s.size() != 2)
		throw configException("Error: invalid client_max_body_size -> " + s[0] + s[1]);
	if (!onlyDigits(s[1]) && s[1].find_first_not_of("0123456789kKmMgG") != std::string::npos)
		throw configException("Error: invalid client_max_body_size -> " + s[0] + s[1]);
	serv.maxRequestSize = s[1];
	parseClientMaxBodySize(serv);
}