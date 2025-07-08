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

void printVector(std::vector<std::string> &s, std::string sep) {
	for (size_t i = 0; i < s.size(); i++)
		std::cout << s[i] << sep;
	std::cout << std::endl;
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

std::string decode(const std::string& encoded) {
    std::string decoded;
    char hex_buf[3] = {0};

    for (std::string::size_type i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            hex_buf[0] = encoded[i + 1];
            hex_buf[1] = encoded[i + 2];

            char decoded_char = static_cast<char>(strtol(hex_buf, NULL, 16));
            decoded += decoded_char;
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
    oss << "[" 
        << (tm_info->tm_year + 1900) << "-"
        << std::setw(2) << std::setfill('0') << (tm_info->tm_mon + 1) << "-"
        << std::setw(2) << std::setfill('0') << tm_info->tm_mday << " "
        << std::setw(2) << std::setfill('0') << tm_info->tm_hour << ":"
        << std::setw(2) << std::setfill('0') << tm_info->tm_min << ":"
        << std::setw(2) << std::setfill('0') << tm_info->tm_sec << "] ";
	if (fd >= 0)
		oss << "[" << fd << "] ";
    
    return oss.str();
}

bool checkReturn(int fd, ssize_t r, const std::string& func, const std::string& isZero) {
	if (r <= 0) {
		if (r < 0)
			std::cerr << getTimeStamp(fd) << RED << "Error: " << func << " failed" << RESET << std::endl;
		else
			std::cerr << getTimeStamp(fd) << RED << isZero << RESET << std::endl;
		return false;
	}
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
			// else if (key == "content" || key == "body" || key == "data" || key == "text" || key == "value")
			// 	fileContent = value;
			else
				fileContent = value;
		}
	}
}

void printConfigs(std::vector<serverLevel> configs) {
	for (size_t i = 0; i < configs.size(); i++) {
		std::cout << "___config[" << i << "]___\n";
		printConfig(configs[i]);
		std::cout << "____________________________________" << std::endl << std::endl;
	}
}

void printConfig(serverLevel& conf) {//only temporary, for debugging
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
			std::cout << conf.port[i].first.first << ":" << conf.port[i].first.second;
			if (conf.port[i].second == true)
				std::cout << " default_server";
			std::cout << std::endl;
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
			std::cout << std::endl;
		}
		std::cout << "\t\tautoindex: ";
		if (its->second.autoindex == true)
			std::cout << "on" << std::endl;
		else
			std::cout << "off" << std::endl;
		// }
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