#pragma once

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <exception>
#include <cstring>
#include <netinet/in.h>

class Webserv {
	private:
		int _tcpSocket;
		sockaddr_in _sockAddr;
		int _rc;
		int _clientSocket;
		bool _err;

	public:
		Webserv();
		Webserv(const Webserv& copy);
		Webserv& operator=(const Webserv& copy);
		~Webserv();

		void setTcpSocket();
		int getTcpSocket() const;
		void setSockAddr();
		sockaddr_in getSockAddr() const;
		void bindSocket();
		int getRc() const;
		int getClientSocket() const;
		void setClientSocket();
		void receiveRequest();
		void loopRequests();
		bool getErr() const;
		void setErr(bool e);
};

class Exception : public std::exception {
	public:
		const char* what() const throw() {
			return "Something isn't ok..";
		}
};
