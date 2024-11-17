#include "../include/config.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

Config::Config(const std::string& filepath) {
    parseFile(filepath);
}

void Config::parseFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open configuration file: " + filepath);
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            _config[key] = value;
        }
    }
}

const std::string& Config::getConfig(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = _config.find(key);
    if (it == _config.end()) {
        throw std::runtime_error("Configuration key not found: " + key);
    }
    return it->second;
}
