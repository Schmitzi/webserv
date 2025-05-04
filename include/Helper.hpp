#ifndef HELPER_HPP
#define HELPER_HPP

#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <map>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

char    *ft_substr(const char *s, unsigned int start, size_t len);
char    **ft_split(const char *s, char c);
std::string ft_itoa(int nbr);
std::vector<std::string> split(const std::string& str, char delimiter);
std::map<std::string, std::string> mapSplit(const std::vector<std::string>& lines);
std::string tostring(int nbr);//like itoa
std::vector<std::string> split(const std::string& s);//like split but only for splitting by spaces

#endif