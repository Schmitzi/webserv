#ifndef NOERRNO_HPP
#define NOERRNO_HPP

#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <errno.h>
#include <ctime>
#include <sys/socket.h>
// #include "Helper.hpp"
// #include "EpollHelper.hpp"

bool checkReturn(int fd, ssize_t r, const std::string& func, const std::string& isZero, bool errIfZero = true);

bool received(int fd, char *buf, size_t len, int flags, std::string &storage, const std::string &isZero, bool errIfZero);
bool sent(int fd, const char *data, size_t len, int flags, ssize_t& bytes, const std::string &isZero, bool errIfZero);
bool wrote(int fd, const char *data, size_t len, ssize_t& bytes, const std::string &isZero, bool errIfZero);
bool readIt(int fd, char *buf, size_t len, ssize_t & bytes, std::string &storage, const std::string &isZero, bool errIfZero);

void safeClose(int &fd);

#endif