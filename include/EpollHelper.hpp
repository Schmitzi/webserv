#ifndef EPOLLHELPER_HPP
#define EPOLLHELPER_HPP

#include <iostream>
#include <map>
#include "Webserv.hpp"
#include "CGIHandler.hpp"

class Webserv;
class CGIHandler;

bool						isCgiPipeFd(Webserv& web, int fd);
void						registerCgiPipe(Webserv& web, int pipe, CGIHandler* handler);
void						unregisterCgiPipe(Webserv& web, int pipe);
CGIHandler*					getCgiHandler(Webserv& web, int fd);

void						addSendBuf(Webserv& web, int fd, const std::string& s);
void						clearSendBuf(Webserv& web, int fd);

int             			addToEpoll(Webserv& web, int fd, short events);
void            			removeFromEpoll(Webserv& web, int fd);
int							setEpollEvents(Webserv& web, int fd, uint32_t events);

#endif