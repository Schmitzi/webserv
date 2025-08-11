#include "../include/Helper.hpp"
#include "../include/ConfigHelper.hpp"
#include "../include/ConfigParser.hpp"
#include "../include/Client.hpp"

std::string tostring(int nbr) {
	std::ostringstream oss;
	oss << nbr;
	std::string result = oss.str();
	return result;
}

std::vector<std::string> split(const std::string& s) {
	std::vector<std::string> ret;
	std::istringstream iss(s);
	std::string single;
	while (iss >> single)
		ret.push_back(single);
	return ret;
}

std::vector<std::string> splitBy(const std::string& str, char div) {
	std::vector<std::string> ret;
	std::istringstream ss(str);
	std::string item;
	while (std::getline(ss, item, div)) {
		if (!item.empty())
			ret.push_back(item);
	}
	return ret;
}

std::string toLower(const std::string& s) {
	std::string ret = "";
	for (size_t i = 0; i < s.size(); i++)
		ret += static_cast<char>(std::tolower(s[i]));
	return ret;
}

size_t iFind(const std::string& haystack, const std::string& needle) {
	if (haystack.empty() || needle.empty())
		return std::string::npos;
	return toLower(haystack).find(toLower(needle));
}

bool iEqual(const std::string& a, const std::string& b) {
	if (toLower(a) == toLower(b))
		return true;
	return false;
}

std::map<std::string, std::string>::iterator iMapFind(std::map<std::string, std::string>& map, const std::string& s) {
	std::map<std::string, std::string>::iterator it = map.begin();
	for (; it != map.end(); ++it) {
		if (iEqual(it->first, s))
			return it;
	}
	return map.end();
}

bool isAbsPath(std::string& path) {
	if (!path.empty() && path.find("http://") == 0)
		return true;
	return false;
}

std::string matchAndAppendPath(const std::string& base, const std::string& add) {
	bool startSlash = false;
	bool endSlash = false;
	if (add.empty() && base[base.size() - 1] == '/')
		endSlash = true;
	else if (add[add.size() - 1] == '/')
		endSlash = true;
	if (base[0] == '/')
		startSlash = true;
	std::vector<std::string> baseParts = splitBy(base, '/');
	std::vector<std::string> addParts = splitBy(add, '/');

	size_t overlap = 0;
	size_t max = std::min(baseParts.size(), addParts.size());

	for (size_t i = 0; i < max; ++i) {
		bool match = true;
		for (size_t j = 0; j <= i; ++j) {
			if (baseParts[baseParts.size() - 1 - i + j] != addParts[j]) {
				match = false;
				break;
			}
		}
		if (match)
			overlap = i + 1;
	}

	std::vector<std::string> preResult = baseParts;
	preResult.insert(preResult.end(), addParts.begin() + overlap, addParts.end());
	std::string result;
	for (size_t i = 0; i < preResult.size(); ++i)
		result = combinePath(result, preResult[i]);
	if (endSlash)
		result += "/";
	if (startSlash)
		result = "/" + result;
	return result;
}

std::string decode(const std::string& encoded) {
	std::string decoded;
	char hexBuf[3] = {0};

	for (std::string::size_type i = 0; i < encoded.length(); ++i) {
		if (encoded[i] == '%' && i + 2 < encoded.length()) {
			hexBuf[0] = encoded[i + 1];
			hexBuf[1] = encoded[i + 2];

			char decodedChar = static_cast<char>(strtol(hexBuf, NULL, 16));
			decoded += decodedChar;
			i += 2;
		} else if (encoded[i] == '+')
			decoded += ' ';
		else
			decoded += encoded[i];
	}
	return decoded;
}

std::string encode(const std::string& decoded) {
	std::ostringstream encoded;

	for (std::string::size_type i = 0; i < decoded.length(); ++i) {
		unsigned char c = decoded[i];
		if (c == '/')
			encoded << '/';
		else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')
				|| c == '-' || c == '_' || c == '.' || c == '~') {
			encoded << c;
		}
		else {
			encoded << '%' << std::uppercase << std::hex
					<< std::setw(2) << std::setfill('0') << int(c);
		}
	}

	return encoded.str();
}

std::string getTimeStamp(int fd) {
	time_t now = time(NULL);
	struct tm* tm_info = localtime(&now);
	
	std::ostringstream oss;
	oss << GREY << "[" 
		<< (tm_info->tm_year + 1900) << "-"
		<< std::setw(2) << std::setfill('0') << (tm_info->tm_mon + 1) << "-"
		<< std::setw(2) << std::setfill('0') << tm_info->tm_mday << " "
		<< std::setw(2) << std::setfill('0') << tm_info->tm_hour << ":"
		<< std::setw(2) << std::setfill('0') << tm_info->tm_min << ":"
		<< std::setw(2) << std::setfill('0') << tm_info->tm_sec << "] ";
	if (fd >= 0)
		oss << "[" << fd << "] " << RESET;
	
	return oss.str();
}

std::string getCurrentTime() {
	time_t now = time(NULL);
	struct tm* tm_info = localtime(&now);

	char buffer[80];
	strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", tm_info);
	return std::string(buffer);
}

bool checkReturn(Client& c, int fd, ssize_t r, const std::string& func, std::string errMsgOnZero) {
	if (r < 0) {
		c.output() += getTimeStamp(fd) + RED + "Error: " + func + " failed\n" + RESET;
		return false;
	} else if (r == 0 && errMsgOnZero != "") {
		c.output() += getTimeStamp(fd) + RED + errMsgOnZero + RESET + "\n";
		return false;
	} else
		return true;
}

void doQueryStuff(const std::string text, std::string& fileName, std::string& fileContent) {
	if (!text.empty()) {
		std::istringstream ss(text);
		std::string pair;

		while (std::getline(ss, pair, '&')) {
			size_t pos = pair.find('=');
			if (pos == std::string::npos)
				continue;
			std::string key = pair.substr(0, pos);
			std::string value = pair.substr(pos + 1);

			if (key == "file" || key == "name" || key == "test")
				fileName = value;
			else
				fileContent = value;
		}
	}
}

bool deleteErrorPages() {
	struct stat info;
	std::string path = "errorPages";

	if (stat(path.c_str(), &info) != 0 || !S_ISDIR(info.st_mode))
		return true;
	DIR* dir = opendir(path.c_str());
	if (!dir) {
		return false;
	}
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string entryName(entry->d_name);
		if (entryName == "." || entryName == "..")
			continue;
		std::string fullPath = path + "/" + entryName;
		if (remove(fullPath.c_str()) != 0) {
			closedir(dir);
			return false;
		}
	}
	closedir(dir);
	if (rmdir(path.c_str()) != 0) {
		return false;
	}
	return true;
}
