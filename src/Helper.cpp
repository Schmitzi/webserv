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

std::string getTimeStamp() {
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
    
    return oss.str();
}