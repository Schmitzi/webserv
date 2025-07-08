#include "../include/Webserv.hpp"
#include <dirent.h>

Webserv* g_webserv;

bool deleteErrorPages() {
	struct stat info;
	std::string path = "errorPages";

	if (stat(path.c_str(), &info) != 0 || !S_ISDIR(info.st_mode))
		return true;
	DIR* dir = opendir(path.c_str());
	if (!dir) {
		return false;
	}
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string entryName(entry->d_name);
		if (entryName == "." || entryName == "..")
			continue;
		std::string fullPath = path + "/" + entryName;
		if (unlink(fullPath.c_str()) != 0) {
			closedir(dir);
			return false;
		}
	}
	closedir(dir);
	if (rmdir(path.c_str()) != 0) {
		return false;
	}
	return true;
}

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\r" << std::string(80, ' ') << "\r" << std::flush;
        std::cout << "Received signal, shutting down...\n";

		if (!deleteErrorPages())
			std::cerr << getTimeStamp() << RED << "Failed to delete error pages directory" << RESET << std::endl;
    
        std::cout << "Goodbye!" << std::endl;
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
    return 0;
}
