#include "../include/webserv.hpp"

int main(int argc, char* argv[]) {
    Webserv webserv(argc > 1 ? argv[1] : "default.conf");  // Use a default config if none provided
    webserv.run();
    return 0;
}