#ifndef CONFIGHELPER_HPP
#define CONFIGHELPER_HPP

#include "Helper.hpp"
#include "ConfigValidator.hpp"
#include "ConfigParser.hpp"

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
void setRootLoc(struct locationLevel& loc, std::vector<std::string>& s);
void setLocIndexFile(struct locationLevel& loc, std::vector<std::string>& s);
void setMethods(struct locationLevel& loc, std::vector<std::string>& s);
void setAutoindex(struct locationLevel& loc, std::vector<std::string>& s);
void setRedirection(struct locationLevel& loc, std::vector<std::string>& s);
void setCgiProcessorPath(struct locationLevel& loc, std::vector<std::string>& s);
void setUploadDirPath(struct locationLevel& loc, std::vector<std::string>& s);

//set Server Level
void setPort(std::vector<std::string>& s, struct serverLevel& serv);
void setRootServ(struct serverLevel& serv, std::vector<std::string>& s);
void setServIndexFile(struct serverLevel& serv, std::vector<std::string>& s);
void setServName(struct serverLevel& serv, std::vector<std::string>& s);
void setErrorPages(std::vector<std::string>& s, struct serverLevel& serv);
void setMaxRequestSize(struct serverLevel& serv, std::vector<std::string>& s);

#endif