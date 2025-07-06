#include "../include/NoErrNo.hpp"

bool checkReturn(int fd, ssize_t r, const std::string& func, const std::string& isZero, bool errIfZero) {
	if (r <= 0) {
		if (r < 0)
			std::cerr << getTimeStamp(fd) << RED << "Error: " << func << " failed" << RESET << std::endl;
		else if (errIfZero == true)
			std::cerr << getTimeStamp(fd) << RED << isZero << RESET << std::endl;
		else {
			std::cout << getTimeStamp(fd) << BLUE << isZero << RESET << std::endl;
			return true;
		}
		return false;
	}
	return true;
}

bool received(int fd, char *buf, size_t len, int flags, std::string& storage, const std::string& isZero, bool errIfZero) {
	std::cout << "RECEIVED" << std::endl;
	if (fd < 0 || len == 0 || !buf) {
		std::cerr << getTimeStamp(fd) << RED << "Error: received() failed" << RESET << std::endl;
		return false;
	}
	ssize_t bytes = recv(fd, buf, len, flags);
	if (!checkReturn(fd, bytes, "recv()", isZero, errIfZero))
		return false;
	storage.append(buf, bytes);
	return true;
}

bool sent(int fd, const char *data, size_t len, int flags, ssize_t& bytes, const std::string& isZero, bool errIfZero) {
	std::cout << "SENT" << std::endl;
	if (fd < 0 || len == 0 || !data) {
		std::cerr << getTimeStamp(fd) << RED << "Error: sent() failed" << RESET << std::endl;
		return false;
	}
	bytes = send(fd, data, len, flags);
	if (!checkReturn(fd, bytes, "send()", isZero, errIfZero))
		return false;
	return true;
}

bool wrote(int fd, const char *data, size_t len, ssize_t& bytes, const std::string& isZero, bool errIfZero) {
	std::cout << "WROTE" << std::endl;
	if (fd < 0 || len == 0 || !data) {
		std::cerr << getTimeStamp(fd) << RED << "Error: wrote() failed" << RESET << std::endl;
		return false;
	}
	bytes = write(fd, data, len);
	if (!checkReturn(fd, bytes, "write()", isZero, errIfZero))
		return false;
	return true;
}

bool readIt(int fd, char *buf, size_t len, ssize_t& bytes, std::string& storage, const std::string& isZero, bool errIfZero) {
	std::cout << "READIT" << std::endl;
	if (fd < 0 || len == 0 || !buf) {
		std::cerr << getTimeStamp(fd) << RED << "Error: readIt() failed" << RESET << std::endl;
		return false;
	}
	bytes = read(fd, buf, len);
	if (!checkReturn(fd, bytes, "read()", isZero, errIfZero))
		return false;
	storage.append(buf, bytes);
	return true;
}

void safeClose(int& fd) {
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}
}