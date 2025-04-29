#ifndef CONFIGVALIDATOR_HPP
#define CONFIGVALIDATOR_HPP

#include "Helper.hpp"
#include "ConfigParser.hpp"

bool isValidPath(const std::string& path);
bool isValidRedirectPath(const std::string &path);
bool isValidDir(const std::string &path);
bool isValidName(const std::string& name);
bool isValidIndexFile(const std::string& indexFile);
void parseClientMaxBodySize(struct serverLevel& serv);

void checkRoot(struct serverLevel& serv);
void checkIndex(struct serverLevel& serv);
void checkConfig(struct serverLevel& serv);
void checkMethods(struct locationLevel& loc);
void initLocLevel(std::vector<std::string>& s, struct locationLevel& loc);

#endif