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
#include <stdbool.h>
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
void checkMethods(struct locationLevel& loc);
void initLocLevel(std::vector<std::string>& s, struct locationLevel& loc);

std::vector<std::string> splitIfSemicolon(std::string& configLine);
void setPort(std::vector<std::string>& s, struct serverLevel& serv);
void setErrorPages(std::vector<std::string>& s, struct serverLevel& serv);
struct locationLevel matchLocation(const std::string& uri, const std::map<std::string, struct locationLevel>& locations);

//set Config Levels
bool foundServer(std::vector<std::string>& s);
bool foundLocation(std::vector<std::string>& s);
void checkBracket(std::vector<std::string>& s, bool& bracket);

//set Location Level
void setRootLoc(struct locationLevel& loc, std::vector<std::string>& s);
void setLocIndexFile(struct locationLevel& loc, std::vector<std::string>& s);
void setMethods(struct locationLevel& loc, std::vector<std::string>& s);
void setAutoindex(struct locationLevel& loc, std::vector<std::string>& s);
void setRedirection(struct locationLevel& loc, std::vector<std::string>& s);
void setCgiProcessorPath(struct locationLevel& loc, std::vector<std::string>& s);
void setUploadDirPath(struct locationLevel& loc, std::vector<std::string>& s);

//set Server Level
void setRootServ(struct serverLevel& serv, std::vector<std::string>& s);
void setServIndexFile(struct serverLevel& serv, std::vector<std::string>& s);
void setServName(struct serverLevel& serv, std::vector<std::string>& s);
void setMaxRequestSize(struct serverLevel& serv, std::vector<std::string>& s);

#endif