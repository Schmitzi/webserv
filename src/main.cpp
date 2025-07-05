#include "../include/Webserv.hpp"

Webserv* g_webserv;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
		g_webserv->flipState();
    }
}


int main(int ac, char **av, char **envp) {
    (void) ac;

    try {
		signal(SIGINT, signalHandler);
		signal(SIGTERM, signalHandler);
	
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
    return 0;
}
