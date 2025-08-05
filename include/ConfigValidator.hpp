#ifndef CONFIGVALIDATOR_HPP
#define CONFIGVALIDATOR_HPP

#include <unistd.h>
#include <string>
#include <vector>

struct	serverLevel;
struct	locationLevel;

bool		isValidPath(std::string& path);
bool		isValidRedirectPath(const std::string &path);
bool		isValidDir(std::string &path);
bool		isValidExecutable(const std::string& path);
bool		isValidName(const std::string& name);
bool		isValidIndexFile(const std::string& indexFile);
void		parseClientMaxBodySize(serverLevel& serv);

void		checkRoot(serverLevel& serv);
void		checkErrorPages(serverLevel& serv);
void		checkUploadDir(serverLevel& serv);
void		checkConfig(serverLevel& serv);
void		checkMethods(locationLevel& loc);
void		initLocLevel(std::vector<std::string>& s, locationLevel& loc);

#endif