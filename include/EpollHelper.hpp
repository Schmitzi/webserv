#ifndef EPOLLHELPER_HPP
#define EPOLLHELPER_HPP

#include <sys/epoll.h>
#include <string>

class	Webserv;
class	CGIHandler;

void	setEpollEvents(Webserv& web, int fd, uint32_t events);
int		addToEpoll(Webserv& web, int fd, short events);
void	removeFromEpoll(Webserv& web, int fd);
void	addSendBuf(Webserv& web, int fd, const std::string& s);
void	clearSendBuf(Webserv& web, int fd);
bool	isCgiPipeFd(Webserv& web, int fd);
void	registerCgiPipe(Webserv& web, int fd, CGIHandler* handler);
void	unregisterCgiPipe(Webserv& web, int fd);

#endif