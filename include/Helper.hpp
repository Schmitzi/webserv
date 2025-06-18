#ifndef HELPER_HPP
#define HELPER_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "ConfigHelper.hpp"

std::string					tostring(int nbr);//like itoa
std::vector<std::string>	split(const std::string& s);//like split but only for splitting by spaces
void						printVector(std::vector<std::string> &s, std::string sep);//temporary
std::vector<std::string>	splitPath(const std::string& path);
std::string					joinPath(const std::vector<std::string>& parts);
std::string					matchAndAppendPath(const std::string& fullPath, const std::string& reqPath);

#endif