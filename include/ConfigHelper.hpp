#ifndef CONFIGHELPER_HPP
#define CONFIGHELPER_HPP

#include <iostream>
#include <cstring>
#include <map>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include "../include/Helper.hpp"
#include "../include/ConfigParser.hpp"

bool onlyDigits(const std::string& s);
bool whiteLine(std::string& line);
bool checkSemicolon(std::string& line);
std::string skipComments(std::string& s);

bool isValidPath(const std::string& path);
bool isValidRedirectPath(const std::string &path);
bool isValidDir(const std::string &path);
bool isValidName(const std::string& name);
bool isValidIndexFile(const std::string& indexFile);
void parseClientMaxBodySize(struct serverLevel& serv);

void checkRoot(struct serverLevel& serv);
void checkIndex(struct serverLevel& serv);
void checkConfig(struct serverLevel& serv);

std::vector<std::string> splitIfSemicolon(std::string& configLine);
void setErrorPages(std::vector<std::string>& s, struct serverLevel& serv);

#endif