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

void printVector(std::vector<std::string> &s, std::string sep) {
	for (size_t i = 0; i < s.size(); i++)
		std::cout << s[i] << sep;
	std::cout << std::endl;
}

std::string matchAndAppendPath(const std::string& base, const std::string& add) {
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
	return result;
}

