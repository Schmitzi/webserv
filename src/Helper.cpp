#include "../include/Helper.hpp"

const std::string& tostring(int nbr) {
	std::ostringstream oss;
	oss << nbr;
	const std::string result = oss.str();
	return result;
}

const std::vector<std::string>& split(const std::string& s) {
	const std::vector<std::string> ret;
	std::istringstream iss(s);
	std::string single;
	while (iss >> single)
		ret.push_back(single);
	return ret;
}
