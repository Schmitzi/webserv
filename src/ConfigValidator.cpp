#include "../include/ConfigValidator.hpp"
#include "../include/Helper.hpp"
#include "../include/ConfigParser.hpp"
#include "../include/ConfigHelper.hpp"

bool isValidPath(std::string &path) {
	struct stat	info;
	return (stat(path.c_str(), &info) == 0 && !S_ISDIR(info.st_mode) && access(path.c_str(), R_OK) == 0);
}

bool isValidRedirectPath(const std::string &path) {
	return (!path.empty() && (path[0] == '/' || path.find("http://") == 0 || path.find("https://") == 0));
}

bool isValidDir(std::string &path) {
	struct stat	info;
	return (stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode) && access(path.c_str(), R_OK) == 0);
}

bool isValidExecutable(const std::string& path) {
	struct stat info;
	if (stat(path.c_str(), &info) != 0) return false;
	if (!S_ISREG(info.st_mode)) return false;
	if (access(path.c_str(), X_OK) != 0) return false;
	return true;
}

bool isValidName(const std::string& name) {
	if (name.empty()) return true;
	if (name.size() > 253) return false;
	if (name[0] == '~') {
		std::cerr << getTimeStamp() << RED << "Regex server names not supported" << RESET << std::endl;
		return false;
	}
	int starCount = 0;
	std::string label;
	for (size_t i = 0; i < name.size(); ++i) {
		char c = name[i];
		if (c == '.') {
			if (label.empty() || label[0] == '-' || label[label.size() - 1] == '-') return false;
			if (label == "*") {
				++starCount;
				if (starCount > 1) return false;
			}
			label.clear();
		}
		else if (isalnum(c) || c == '-') label += c;
		else if (c == '*') {
			label += c;
			if (label != "*") return false;
		}
		else return false;
	}
	if (label.empty() || label[0] == '-' || label[label.size() - 1] == '-') return false;
	if (label == "*") {
		++starCount;
		if (starCount > 1) return false;
	}
	if (starCount == 1)
		if (!(name.find("*.") == 0 || name.rfind(".*") == name.size() - 2)) return false;
	return true;
}

bool isValidIndexFile(const std::string &indexFile) {
	if (indexFile.empty() || indexFile.find("..") != std::string::npos)
		return false;
	std::string::size_type dot = indexFile.rfind('.');
	if (dot == std::string::npos)
		return false;
	std::string ext = indexFile.substr(dot);
	return ext == ".html";
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
	std::map<std::string, locationLevel>::iterator it = serv.locations.begin();
	while (it != serv.locations.end()) {
		if (it->second.rootLoc.empty() && serv.rootServ.empty())
			throw configException("Error: No rootLoc found.");
		else if (it->second.rootLoc.empty())
			it->second.rootLoc = serv.rootServ;
		++it;
	}
}

void checkErrorPages(serverLevel& serv) {
	std::map<std::vector<int>, std::string>::iterator it = serv.errPages.begin();
	for (; it != serv.errPages.end(); ++it) {
		std::string path = matchAndAppendPath(serv.rootServ, it->second);
		if (!isValidPath(path))
				throw configException("Error: Invalid path (error_page) -> " + path);
		it->second = path;
	}
}

void checkUploadDir(serverLevel& serv) {
	std::map<std::string, locationLevel>::iterator it = serv.locations.begin();
	for (; it != serv.locations.end(); ++it) {
		if (!it->second.uploadDirPath.empty()) {
			std::string path = matchAndAppendPath(it->second.rootLoc, it->second.uploadDirPath);
			it->second.uploadDirPath = path;
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
	checkRoot(serv);
	checkErrorPages(serv);
	checkUploadDir(serv);
}

void checkMethods(locationLevel& loc) {
	if (loc.methods.empty()) {
		loc.methods.push_back("GET");
		loc.methods.push_back("POST");
		loc.methods.push_back("DELETE");
	}
}

void initLocLevel(std::vector<std::string>& s, locationLevel& loc) {
	if (s.size() == 4 && s[1] == "~" && s[2][0] == '\\' && s[2][s[2].size() - 1] == '$') {
		loc.isRegex = true;
		for (size_t x = 2; x < s.size() && s[x] != "{"; x++) {
			for (size_t y = 0; y < s[x].size(); y++) {
				if (s[x][y] != '\\' && s[x][y] != '$')
					loc.locName += s[x][y];
			}
		}
	} else {
		for (size_t x = 1; x < s.size() && s[x] != "{"; x++) {
			s[x] = decode(s[x]);
			loc.locName += s[x];
			if (x < s.size() - 1 && s[x + 1] != "{")
				loc.locName += " ";
		}
	}
}
