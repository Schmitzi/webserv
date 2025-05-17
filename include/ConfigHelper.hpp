#ifndef CONFIGHELPER_HPP
#define CONFIGHELPER_HPP

#include "Helper.hpp"
#include "ConfigValidator.hpp"
#include "ConfigParser.hpp"

struct serverLevel;
struct locationLevel;
struct portLevel;
class ConfigParser;

bool onlyDigits(const std::string& s);
bool whiteLine(std::string& line);
bool checkSemicolon(std::string& line);
std::string skipComments(std::string& s);
std::vector<std::string> splitIfSemicolon(std::string& configLine);

//set Config Levels
bool foundServer(std::vector<std::string>& s);
bool foundLocation(std::vector<std::string>& s);
void checkBracket(std::vector<std::string>& s, bool& bracket);

//set Location Level
void setRootLoc(locationLevel& loc, std::vector<std::string>& s);
void setLocIndexFile(locationLevel& loc, std::vector<std::string>& s);
void setMethods(locationLevel& loc, std::vector<std::string>& s);
void setAutoindex(locationLevel& loc, std::vector<std::string>& s);
void setRedirection(locationLevel& loc, std::vector<std::string>& s);
void setCgiProcessorPath(locationLevel& loc, std::vector<std::string>& s);
void setUploadDirPath(locationLevel& loc, std::vector<std::string>& s);

//set Server Level
void setPort(std::vector<std::string>& s, serverLevel& serv);
void setRootServ(serverLevel& serv, std::vector<std::string>& s);
void setServIndexFile(serverLevel& serv, std::vector<std::string>& s);
void setServName(serverLevel& serv, std::vector<std::string>& s);
void setErrorPages(std::vector<std::string>& s, serverLevel& serv);
void setMaxRequestSize(serverLevel& serv, std::vector<std::string>& s);

#endif