#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <netinet/in.h>

class Client {
    public:
        struct sockaddr_in  _addr;
        socklen_t           _addrLen;
        int                 _fd;
        unsigned char       *ip;
};

#endif