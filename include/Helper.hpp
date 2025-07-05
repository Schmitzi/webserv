#ifndef HELPER_HPP
#define HELPER_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <iomanip>
#include "ConfigHelper.hpp"

struct	serverLevel;

std::string					tostring(int nbr);
std::vector<std::string>	split(const std::string& s);
void						printVector(std::vector<std::string> &s, std::string sep);
std::vector<std::string>	splitBy(const std::string& str, char div);
std::string					matchAndAppendPath(const std::string& fullPath, const std::string& reqPath);
std::string					decode(const std::string& encoded);
std::string					encode(const std::string& decoded);
std::string					getTimeStamp(int fd = -1);
void						doQueryStuff(const std::string text, std::string& fileName, std::string& fileContent);
void						printConfigs(std::vector<serverLevel> configs);
void						printConfig(serverLevel& conf);

#endif