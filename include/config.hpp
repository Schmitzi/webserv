#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <map>

class Config {
    public:
        Config();
        Config(const std::string& filepath);
        Config& operator=(Config C);
        ~Config();
        const std::string& getConfig(const std::string& key) const;
    private:
        std::map<std::string, std::string> _config;
        void parseFile(const std::string& filepath);
};

#endif
