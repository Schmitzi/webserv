#include "../include/Helper.hpp"

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
	while (iss >> single) {
		ret.push_back(single);
	}
	return ret;
}

void printVector(std::vector<std::string> &s, std::string sep) {
	for (size_t i = 0; i < s.size(); i++)
		std::cout << s[i] << sep;
	std::cout << std::endl;
}

std::vector<std::string> splitPath(const std::string& path) {
	std::vector<std::string> parts;
	std::stringstream ss(path);
	std::string item;
	while (std::getline(ss, item, '/')) {
		if (!item.empty())
			parts.push_back(item);
	}
	return parts;
}

std::string joinPath(const std::vector<std::string>& parts) {
	std::string result;
	for (size_t i = 0; i < parts.size(); ++i)
		result = combinePath(result, parts[i]);
	return result;
}

std::string matchAndAppendPath(const std::string& fullPath, const std::string& reqPath) {
	std::vector<std::string> baseParts = splitPath(fullPath);
	std::vector<std::string> reqParts = splitPath(reqPath);

	size_t overlap = 0;
	size_t max = std::min(baseParts.size(), reqParts.size());

	for (size_t i = 0; i < max; ++i) {
		bool match = true;
		for (size_t j = 0; j <= i; ++j) {
			if (baseParts[baseParts.size() - 1 - i + j] != reqParts[j]) {
				match = false;
				break;
			}
		}
		if (match)
			overlap = i + 1;
	}

	std::vector<std::string> result = baseParts;
	result.insert(result.end(), reqParts.begin() + overlap, reqParts.end());
	return joinPath(result);
}

