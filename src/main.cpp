#include "../include/Webserv.hpp"
Webserv* g_webserv = NULL;

int    ft_error(std::string const errMsg) {
    std::cerr << "Error: " << errMsg << "\n";
    return 0;
}

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\r" << std::string(80, ' ') << "\r" << std::flush;
        std::cout << "Received signal, shutting down...\n";

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
    (void) av;
    (void) ac;

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    Webserv* webserv = new Webserv();
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

    return 0;
}
