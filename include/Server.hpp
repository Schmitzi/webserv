#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>

class Server {
    public:
        int _fd;
        struct sockaddr_in _addr;
};

#endif