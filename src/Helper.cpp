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