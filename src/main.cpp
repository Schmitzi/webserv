#include "../include/Webserv.hpp"

int    ft_error(std::string const errMsg) {
    std::cerr << "Error: " << errMsg << "\n";
    return 0;
}

int inputCheck(int ac, char **av, Webserv &webserv) {
    if (ac > 2) {
        return (ft_error("Bad arguments"));
    } else if (ac == 1) {
        if (webserv.setConfig("../config/default.conf")) {
            std::cout << "Config saved\n";
        }
    } else {
        webserv.setConfig(av[1]);
    }
    return 0;
}

int main(int ac, char **av) {
    (void) av;
    Webserv webserv;

    if (inputCheck(ac, av, webserv)) {
        return -1;
    }
     
    //webserv.run();
    return 0;
}