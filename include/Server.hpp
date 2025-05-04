#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <arpa/inet.h>

#define BLUE    "\33[34m"
#define GREEN   "\33[32m"
#define RED     "\33[31m"
#define WHITE   "\33[97m"
#define RESET   "\33[0m" // No Colour

// Forward declarations
class Webserv;
class Config;

class Server {
    public:
        Server();
        Server(const Server& other);
        Server& operator=(const Server& other);
        ~Server();
        
        Webserv             &getWebServ();
        struct sockaddr_in  &getAddr();
        int                 &getFd();
        std::string const   &getUploadDir();
        std::string const   &getWebRoot();
        void                setFd(int const fd);
        void                setWebserv(Webserv* webserv);
        void                setConfig(Config* config);
        Config*             getConfig() const;
        int                 openSocket();
        int                 setOptional();
        int                 setServerAddr();
        int                 ft_bind();
        int                 ft_listen();
    private:
        int                 _fd;
        struct sockaddr_in  _addr;
        std::string         _uploadDir;
        std::string         _webRoot;
        Webserv             *_webserv;
        Config              *_config;
};

#endif