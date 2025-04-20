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
	if (x != std::string::npos)
		ret = s.substr(0, x);
	return (ret);
}

bool isValidPath(const std::string &path) {
	struct stat	info;

	return (stat(path.c_str(), &info) == 0 && !S_ISDIR(info.st_mode) && access(path.c_str(), R_OK) == 0);
}

bool isValidRedirectPath(const std::string &path) {
	return (!path.empty() && path[0] == '/' && path.find("http") == 0);
	// if (path.empty() || (path[0] != '/' && path.find("http") != 0))
	// 	return (false);
	// return (true);
}

bool isValidDir(const std::string &path) {
	struct stat	info;

	return (stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode) && access(path.c_str(), R_OK | X_OK) == 0);
}

bool isValidName(const std::string &name) {
	char c;

	// TODO: check how server_names work!!
	if (name.empty())
		return (true);
	if (name.size() > 253)
		return (false);
	std::string::const_iterator it = name.begin();
	std::string label;
	for (; it != name.end(); ++it) {
		c = *it;
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

bool isValidIndexFile(const std::string &indexFile) {
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
	return ext == ".html"; // TODO: maybe add more extension checks?
}

void parseClientMaxBodySize(struct serverLevel &serv) {
	size_t multiplier;
	char unit;
	size_t num;

	multiplier = 1;
	unit = serv.maxRequestSize[serv.maxRequestSize.size() - 1];
	std::string numberPart = serv.maxRequestSize;
	if (!std::isdigit(unit)) {
		switch (std::tolower(unit)) {
			case 'k':
				multiplier = 1024;
				break ;
			case 'm':
				multiplier = 1024 * 1024;
				break ;
			case 'g':
				multiplier = 1024 * 1024 * 1024;
				break ;
			default:
				throw configException("Invalid size unit for client_max_body_size");
		}
		numberPart = serv.maxRequestSize.substr(0, serv.maxRequestSize.size() - 1);
	}
	num = std::strtoul(numberPart.c_str(), NULL, 10);
	serv.requestLimit = num * multiplier;
}

void checkRoot(struct serverLevel &serv) {
	if (serv.rootServ.empty()) {
		std::map<std::string, struct locationLevel>::iterator it = serv.locations.begin();
		while (it != serv.locations.end()) {
			if (it->second.rootLoc.empty())
				throw configException("Error: No root for server and locations specified.\n-> Server doesn’t know where to serve files from");
			++it;
		}
	}
}

void checkIndex(struct serverLevel &serv) {
	if (serv.indexFile.empty()) {
		std::map<std::string, struct locationLevel>::iterator it = serv.locations.begin();
		while (it != serv.locations.end()) {
			if (it->second.indexFile.empty())
				throw configException("Error: No default index for server and locations specified.\n-> Requests to / may return 403 or 404");
			++it;
		}
	}
}

void checkConfig(struct serverLevel &serv) {
	if (serv.port < 0)
		throw configException("Error: No port specified in config.\n-> server won't bind to any port");
	checkRoot(serv);
	checkIndex(serv);
	// if (serv.servName.empty())
	// 	throw configException("Error: No server_name specified in config.\n-> Virtual hosting may break / default fallback");
	// TODO: does every serverLevel need at least one location? (->recommended, but not needed, but maybe needed for out server??)
	// std::map<std::string, struct locationLevel>::iterator it = serv.locations.begin();
	// if (it == serv.locations.end())
	// 	throw configException("Error: no locations specified.\n-> All URL requests fall back to root config");
}

std::vector<std::string> splitIfSemicolon(std::string &configLine) {
	std::vector<std::string> s;
	if (!checkSemicolon(configLine))
		throw configException("Error: missing semicolon in config");
	configLine = configLine.substr(0, configLine.size() - 1);
	s = split(configLine);
	return s;
}

void setErrorPages(std::vector<std::string> &s, struct serverLevel &serv) {
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
