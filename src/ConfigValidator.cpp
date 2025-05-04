#include "../include/ConfigValidator.hpp"

std::string getAbsPath(std::string& path) {
	if (path.empty())
		return (".");
	if (path[0] == '/')
		return (path);
	std::string absPath = getcwd(NULL, 0);
	if (absPath.empty())
		return (".");
	absPath += "/" + path;
	if (absPath[absPath.size() - 1] == '/')
		absPath = absPath.substr(0, absPath.size() - 1);
	return (absPath);
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

	return (stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode) && access(path.c_str(), R_OK) == 0);
}

bool isValidName(const std::string& name) {
	if (name.empty())
		return true;//TODO: can be empty?
	if (name.size() > 253)
		return false;
	if (name[0] == '~') {//TODO: should we support regex server_names?
		std::cerr << "Regex server names not supported" << std::endl;
		return false;
	}
	int starCount = 0;
	std::string label;
	for (size_t i = 0; i < name.size(); ++i) {
		char c = name[i];
		if (c == '.') {
			if (label.empty() || label[0] == '-' || label[label.size() - 1] == '-')
				return false;
			if (label == "*") {
				++starCount;
				if (starCount > 1)
					return false;
			}
			label.clear();
		}
		else if (isalnum(c) || c == '-')
			label += c;
		else if (c == '*') {
			label += c;
			if (label != "*")
				return false;
		}
		else
			return false;
	}
	if (label.empty() || label[0] == '-' || label[label.size() - 1] == '-')
		return false;
	if (label == "*") {
		++starCount;
		if (starCount > 1)
			return false;
	}
	if (starCount == 1) {
		if (!(name.find("*.") == 0 || name.rfind(".*") == name.size() - 2))
			return false;
	}
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

void parseClientMaxBodySize(serverLevel &serv) {
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
				break;
			case 'm':
				multiplier = 1024 * 1024;
				break;
			case 'g':
				multiplier = 1024 * 1024 * 1024;
				break;
			default:
				throw configException("Invalid size unit for client_max_body_size");
		}
		numberPart = serv.maxRequestSize.substr(0, serv.maxRequestSize.size() - 1);
	}
	num = std::strtoul(numberPart.c_str(), NULL, 10);
	serv.requestLimit = num * multiplier;
}

void checkRoot(serverLevel &serv) {
	if (serv.rootServ.empty()) {
		serv.rootServ = "./www";//TODO: should this be the default?
		std::map<std::string, locationLevel>::iterator it = serv.locations.begin();
		while (it != serv.locations.end()) {
			if (it->second.rootLoc.empty())
				it->second.rootLoc = serv.rootServ;//take default value from server if not specified
			++it;
		}
	}
}

void checkIndex(serverLevel &serv) {
	if (serv.indexFile.empty()) {
		std::map<std::string, locationLevel>::iterator it = serv.locations.begin();
		while (it != serv.locations.end()) {
			if (it->second.indexFile.empty())
				throw configException("Error: No default index for server and locations specified.\n-> Requests to / may return 403 or 404");
			++it;
		}
	}
	else if (!serv.indexFile.empty()) {
		std::map<std::string, locationLevel>::iterator it = serv.locations.begin();
		while (it != serv.locations.end()) {
			if (it->second.indexFile.empty())
				it->second.indexFile = serv.indexFile;//take default value from server if not specified
			++it;
		}
	}
}

void checkConfig(serverLevel &serv) {
	if (serv.servName.empty())
		serv.servName.push_back("");
	if (serv.maxRequestSize.empty()) {
		serv.maxRequestSize = "1M";
		parseClientMaxBodySize(serv);
	}
	// throw configException("Error: No port specified in config.\n-> server won't bind to any port");
	checkRoot(serv);
	checkIndex(serv);
}

void checkMethods(locationLevel& loc) {
	if (loc.methods.empty()) {
		loc.methods.push_back("GET");
		loc.methods.push_back("POST");
		loc.methods.push_back("DELETE");
	}
}

void initLocLevel(std::vector<std::string>& s, locationLevel& loc) {
	loc.autoindex = false;
	loc.autoindexFound = false;
	for (size_t x = 1; x < s.size() && s[x] != "{"; x++) {
		loc.locName += s[x];
		if (x < s.size() - 1 && s[x + 1] != "{")
			loc.locName += " ";
	}
}
