#ifndef CONFIGVALIDATOR_HPP
#define CONFIGVALIDATOR_HPP

#include "Helper.hpp"
#include "ConfigParser.hpp"
#include <unistd.h>

struct	serverLevel;
struct	locationLevel;

std::string	getAbsPath(std::string& path);
void		createLocationFromIndex(std::string& path);
bool		isValidPath(std::string& path);
bool		isValidRedirectPath(const std::string &path);
bool		isValidDir(std::string &path);
bool		isValidName(const std::string& name);
bool		isValidIndexFile(const std::string& indexFile);
void		parseClientMaxBodySize(serverLevel& serv);

void		checkRoot(serverLevel& serv);
void		checkIndex(serverLevel& serv);
void		checkConfig(serverLevel& serv);
void		checkMethods(locationLevel& loc);
void		initLocLevel(std::vector<std::string>& s, locationLevel& loc);

#endif