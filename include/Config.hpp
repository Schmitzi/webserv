#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <iostream>
#include <cstring>
#include <map>

class Config {
    public:
        Config();
        Config(const std::string& filepath);
        Config &operator=(Config const &other);
        ~Config();
        std::map<std::string, std::string> const &getConfig() const;
    private:
        std::map<std::string, std::string> _config;
        //void parseFile(std::string const &filepath);
};

#endif
