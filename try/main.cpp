#include "try.hpp"

int main() {
	try {
		Webserv webs = Webserv();
		webs.setTcpSocket();
		webs.setSockAddr();
		webs.bindSocket();
		webs.loopRequests();
		close(webs.getTcpSocket());
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
    }
	return 0;
}