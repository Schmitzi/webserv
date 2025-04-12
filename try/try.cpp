#include "try.hpp"

Webserv::Webserv() {
	_tcpSocket = 0;
	memset(&_sockAddr, 0, sizeof(_sockAddr));
	_rc = 0;
	_clientSocket = 0;
	_err = false;
}

Webserv::Webserv(const Webserv& copy) {
	*this = copy;
}

Webserv& Webserv::operator=(const Webserv& copy) {
	if (this != &copy) {
		this->_tcpSocket = copy.getTcpSocket();
		this->_sockAddr = copy.getSockAddr();
		this->_rc = copy.getRc();
		this->_clientSocket = copy.getClientSocket();
		this->_err = copy.getErr();
	}
	return *this;
}

Webserv::~Webserv() {}

/* ****************************************************************** */

void Webserv::setTcpSocket() {
	_tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_tcpSocket < 0) {
		std::cout << "here\n";
		throw Exception();
	}
}

int Webserv::getTcpSocket() const {
	return _tcpSocket;
}

void Webserv::setSockAddr() {
	_sockAddr.sin_port = htons(8080);
	_sockAddr.sin_family = AF_INET;
	_sockAddr.sin_addr.s_addr = INADDR_ANY;
}

sockaddr_in Webserv::getSockAddr() const {
	return _sockAddr;
}

void Webserv::bindSocket() {
	_rc = bind(_tcpSocket, reinterpret_cast<sockaddr*>(&_sockAddr), sizeof(_sockAddr));
	if (_rc < 0) {
		setErr(true);
		std::cout << "here1\n";
		throw Exception();
	}
}

int Webserv::getRc() const {
	return _rc;
}

int Webserv::getClientSocket() const {
	return _clientSocket;
}

void Webserv::setClientSocket() {
	if (listen(getTcpSocket(), SOMAXCONN) < 0) {
		std::cout << "Failed to listen on socket\n";
		setErr(true);
		throw Exception();
	}
	_clientSocket = accept(getTcpSocket(), NULL, NULL);
	if (_clientSocket < 0) {
		std::cout << "here2\n";
		setErr(true);
		throw Exception();
	}
}

void Webserv::receiveRequest() {
	char buffer[1024];
	ssize_t n = recv(_clientSocket, buffer, sizeof(buffer) - 1, 0);
	if (n < 0) {
		setErr(true);
		std::cout << "here3\n";
		throw Exception();
	}
	std::cout << buffer << std::endl;
}

void Webserv::loopRequests() {
	while (1) {
		setClientSocket();
		receiveRequest();
		if (getErr() == true)
			break;
	}
	close(getClientSocket());
}

bool Webserv::getErr() const {
	return _err;
}

void Webserv::setErr(bool e) {
	_err = e;
}