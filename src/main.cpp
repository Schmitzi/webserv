#include "../include/Webserv.hpp"
#include <dirent.h>

Webserv* g_webserv = NULL;

int    ft_error(std::string const errMsg) {
    std::cerr << "Error: " << errMsg << "\n";
    return 0;
}

bool deleteErrorPages() {
	struct stat info;
	std::string path = "errorPages";

	if (stat(path.c_str(), &info) != 0 || !S_ISDIR(info.st_mode))
		return true;
	DIR* dir = opendir(path.c_str());
	if (!dir) {
		std::cerr << "Failed to delete error pages directory: " << strerror(errno) << std::endl;
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
			std::cerr << "Failed to delete error pages directory: " << strerror(errno) << std::endl;
			return false;
		}
	}
	closedir(dir);
	if (rmdir(path.c_str()) != 0) {
		std::cerr << "Failed to delete error pages directory: " << strerror(errno) << std::endl;
		return false;
	}
	return true;
}

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\r" << std::string(80, ' ') << "\r" << std::flush;
        std::cout << "Received signal, shutting down...\n";

		if (!deleteErrorPages())
			std::cerr << "Failed to delete error pages directory.\n";
        if (g_webserv) {
            delete g_webserv;
            g_webserv = NULL;
        }
    
        std::cout << "Goodbye!" << std::endl;
        std::exit(0); // TEMPORARY!!!!
    }
}

int inputCheck(int ac, char **av, Webserv &webserv) {
    if (ac > 2) {
        return (ft_error("Bad arguments"));
    } else if (ac == 1) {
        if (webserv.setConfig("../config/default.conf")) {
            std::cout << webserv.getTimeStamp() << "Config saved\n";
        }
    } else {
        webserv.setConfig(av[1]);
    }
    return 0;
}

int main(int ac, char **av, char **envp) {
    (void) ac;

    try {
		signal(SIGINT, signalHandler);
		signal(SIGTERM, signalHandler);
	
		Webserv* webserv;
		if (av[1])
			webserv = new Webserv(av[1]);
		else 
			webserv = new Webserv();

		//	webserv->setConfig(av[1]);
		g_webserv = webserv;
	
		webserv->setEnvironment(envp);
	
		// if (inputCheck(ac, av, webserv)) {
		//     return -1;
		// }
	
		if (webserv->run()) {
			webserv->ft_error("Setup failed");
		}
	
		delete webserv;
		g_webserv = NULL;
	} catch (const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}
    return 0;
}
