#ifndef HELPER_HPP
#define HELPER_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <iomanip>
#include <dirent.h>
#include "Colors.hpp"

struct	serverLevel;

std::string					tostring(int nbr);
std::vector<std::string>	split(const std::string& s);
std::vector<std::string>	splitBy(const std::string& str, char div);
std::string					matchAndAppendPath(const std::string& fullPath, const std::string& reqPath);
std::string					decode(const std::string& encoded);
std::string					encode(const std::string& decoded);
std::string					getTimeStamp(int fd = -1);
bool 						checkReturn(int fd, ssize_t r, const std::string& func, std::string errMsgOnZero = "");
void						doQueryStuff(const std::string text, std::string& fileName, std::string& fileContent);
bool						deleteErrorPages();

//extras
void						printConfigs(std::vector<serverLevel> configs);
void						printConfig(serverLevel& conf);

#endif
