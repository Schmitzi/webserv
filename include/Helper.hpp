#ifndef HELPER_HPP
#define HELPER_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

char    *ft_substr(const char *s, unsigned int start, size_t len);
char    **ft_split(const char *s, char c);
std::string ft_itoa(int nbr);

std::string tostring(int nbr);//like itoa
std::vector<std::string> split(const std::string& s);//like split but only for splitting by spaces

#endif