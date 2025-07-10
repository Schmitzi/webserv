#ifndef CLIENTHELPER_HPP
#define CLIENTHELPER_HPP

#include <string>
#include <fcntl.h>

class	Client;
class	Request;

int				checkLength(std::string& reqBuf, int fd, bool &printNewLine);
bool			isChunkedBodyComplete(const std::string& buffer);
bool			isFileBrowserRequest(const std::string& path);
bool			isCGIScript(const std::string& path);
int				buildBody(Client& c, Request &req, std::string fullPath);
bool			ensureUploadDirectory(Client& c, Request& req);
bool			isChunkedRequest(Request& req);
std::string		getLocationPath(Client& c, Request& req, const std::string& method);
std::string		decodeChunkedBody(int fd, const std::string& chunkedData);
bool			endsWith(const std::string& str, const std::string& suffix);
bool			tryLockFile(const std::string& path, int timeStampFd);
void			releaseLockFile(const std::string& path);

#endif