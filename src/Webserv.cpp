#include "../include/Webserv.hpp"

// Othodox Cannonical Form

Webserv::Webserv() {
    std::cout << "Webserv constructed" << std::endl;
}

Webserv::Webserv(std::string const &config) {
   std::cout << "Webserv copy constructed" << std::endl;
   _config = config;
}

Webserv::Webserv(Webserv const &other) {
    _config = other._config;
}

Webserv &Webserv::operator=(Webserv const &other) {
    _config = other._config;
	return *this;
}

Webserv::~Webserv() {
    std::cout << "Webserv deconstructed" << std::endl;
}

int Webserv::setConfig(std::string const filepath) {
    std::cout << "Config found at " << filepath << "\n";
    return true;
}