#include "../include/EpollHelper.hpp"
#include "../include/Webserv.hpp"
#include "../include/CGIHandler.hpp"
#include "../include/NoErrNo.hpp"

bool isCgiPipeFd(Webserv& web, int fd) {
	std::map<int, CGIHandler*>::iterator it = web.getCgis().begin();
	for (; it != web.getCgis().end(); ++it) {
		if (it->first == fd)
			return true;
	}
	return false;
}

void registerCgiPipe(Webserv& web, int pipe, CGIHandler *handler) {
	// std::cout << "\ninside registerCgiPipe, fd: " << pipe << std::endl;
	if (isCgiPipeFd(web, pipe)) {
		std::cerr << getTimeStamp(pipe) << RED << "Error: Pipe already registered" << RESET << std::endl;
		return;
	}
	if (pipe < 0) {
		std::cerr << getTimeStamp(pipe) << RED << "Error: Invalid pipe" << RESET << std::endl;
		return;
	}
	if (web.getEpollFd() < 0) {
		std::cerr << getTimeStamp(pipe) << RED << "Error: Invalid epoll file descriptor" << RESET << std::endl;
		return;
	}
	web.getCgis().insert(std::pair<int, CGIHandler*>(pipe, handler));
	// web.printCgis();
}

CGIHandler* getCgiHandler(Webserv& web, int fd) {
	std::map<int, CGIHandler*>::const_iterator it = web.getCgis().begin();
	for (; it != web.getCgis().end(); ++it) {
		if (it->first == fd)
			return it->second;
	}
	return NULL;
}

void unregisterCgiPipe(Webserv& web, int& fd) {
	std::map<int, CGIHandler*>::iterator it = web.getCgis().find(fd);
	if (it != web.getCgis().end()) {
		removeFromEpoll(web, fd);
		safeClose(fd);
		// delete it->second;
		web.getCgis().erase(it);
		std::cout << getTimeStamp(fd) << " Unregistered CGI pipe" << std::endl;
	}
}

void addSendBuf(Webserv& web, int fd, const std::string& s) {
	web.getSendBuf(fd) += s;
}

void clearSendBuf(Webserv& web, int fd) {
	std::map<int, std::string>::iterator it = web.getSendBuf().begin();
	for (; it != web.getSendBuf().end(); ++it) {
		if (it->first == fd) {
			web.getSendBuf().erase(it);
			return;
		}
	}
}

int setEpollEvents(Webserv& web, int& fd, uint32_t events) {
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

int addToEpoll(Webserv& web, int& fd, short events) {  
	struct epoll_event event;
	event.events = events;
	event.data.fd = fd;
	if (epoll_ctl(web.getEpollFd(), EPOLL_CTL_ADD, fd, &event) == -1) {
		std::cerr << getTimeStamp(fd) << RED << "Error: epoll_ctl ADD failed" << RESET << std::endl;
		return 1;
	}
	return 0;
}

void removeFromEpoll(Webserv& web, int& fd) {
	if (web.getEpollFd() < 0 || fd < 0)
		return;
	if (epoll_ctl(web.getEpollFd(), EPOLL_CTL_DEL, fd, NULL) == -1) {
		if (errno != EBADF && errno != ENOENT)
		std::cerr << getTimeStamp(fd) << RED << "Warning: epoll_ctl DEL failed" << RESET << std::endl;
	}
	std::cout << getTimeStamp(fd) << "Removed from Epoll" << std::endl;
}