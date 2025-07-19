#include "../include/Webserv.hpp"
#include "../include/Helper.hpp"

Webserv* g_webserv;

void signalHandler(int signal) {
	if (signal == SIGINT || signal == SIGTERM) {
		std::cout << "\r" << std::string(80, ' ') << "\r" << std::flush;
		std::cout << CYAN << "________________Received signal, cleaning up...________________" << RESET << std::endl;

		if (!deleteErrorPages())
			std::cerr << getTimeStamp() << RED << "Failed to delete error pages directory" << RESET << std::endl;

		g_webserv->flipState();
	}
}


int main(int ac, char **av, char **envp) {
	(void) ac;

	try {
		signal(SIGINT, signalHandler);
		signal(SIGTERM, signalHandler);
		signal(SIGPIPE, SIG_IGN);
	
		if (av[1]) {
			Webserv webserv = Webserv(av[1]);
			g_webserv = &webserv;
			webserv.setEnvironment(envp);
			if (webserv.run())
				std::cerr << getTimeStamp() << RED << "Error: setup/run failed" << RESET << std::endl;
		}
		else {
			Webserv webserv = Webserv();
			g_webserv = &webserv;
			webserv.setEnvironment(envp);
			if (webserv.run())
				std::cerr << getTimeStamp() << RED << "Error: setup/run failed" << RESET << std::endl;
		}
	} catch (const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}
	std::cout << CYAN << "-------------------------Goodbye!-------------------------" << RESET << std::endl;
	return 0;
}
