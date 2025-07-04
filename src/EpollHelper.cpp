#include "../include/EpollHelper.hpp"

bool isCgiPipeFd(Webserv& web, int fd) {
    return web.getCgis().find(fd) != web.getCgis().end();
}

void registerCgiPipe(Webserv& web, int pipe, CGIHandler* handler) {
    web.getCgis()[pipe] = handler;
}

CGIHandler* getCgiHandler(Webserv& web, int fd) {
    std::map<int, CGIHandler*>::const_iterator it = web.getCgis().find(fd);
    if (it != web.getCgis().end())
        return it->second;
    return NULL;
}

void unregisterCgiPipe(Webserv& web, int pipe) {
    web.getCgis().erase(pipe);
}

void addSendBuf(Webserv& web, int fd, const std::string& s) {
	web.getSendBuf(fd) += s;
}

void clearSendBuf(Webserv& web, int fd) {
	web.getSendBuf().erase(fd);
}

int setEpollEvents(Webserv& web, int fd, uint32_t events) {
	if (fd < 0)
		return 1;
	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd;
	if (epoll_ctl(web.getEpollFd(), EPOLL_CTL_MOD, fd, &ev) == -1) {
		std::cerr << getTimeStamp(fd) << RED << "Error: epoll_ctl MOD failed" << RESET << std::endl;
		return 1;
	}
	return 0;
}

int addToEpoll(Webserv& web, int fd, short events) {  
    struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    if (epoll_ctl(web.getEpollFd(), EPOLL_CTL_ADD, fd, &event) == -1) {
		std::cerr << getTimeStamp(fd) << RED << "Error: epoll_ctl ADD failed" << RESET << std::endl;
        return 1;
    }
    return 0;
}

void removeFromEpoll(Webserv& web, int fd) {
	if (web.getEpollFd() < 0 || fd < 0)
		return;
    if (epoll_ctl(web.getEpollFd(), EPOLL_CTL_DEL, fd, NULL) == -1) {
		if (errno != EBADF && errno != ENOENT)
		std::cerr << getTimeStamp(fd) << RED << "Warning: epoll_ctl DEL failed" << RESET << std::endl;
    }
	std::cout << getTimeStamp(fd) << "Removed from Epoll" << std::endl;
}