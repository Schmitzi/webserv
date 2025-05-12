#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <iostream>
#include <vector>
#include <sys/wait.h>
#include <cstdlib>
// #include <fcntl.h>
#include <cerrno>
#include "../include/Request.hpp"
#include "../include/Helper.hpp"
#include "../include/Config.hpp"

class Client;
class Server;
class Config;

class CGIHandler {
    public:
        CGIHandler();
        ~CGIHandler();
        void    setClient(Client &client);
        void    setServer(Server &server);
        void    setConfig(Config config);
        bool    isCGIScript(const std::string& path);
        int     doChecks(Client client);
        int     processScriptOutput(Client &client);
        int     handleStandardOutput(const std::map<std::string, std::string>& headerMap, const std::string& initialBody);
        int     handleChunkedOutput(const std::map<std::string, std::string>& headerMap, const std::string& initialBody);
        int     executeCGI(Client &client, Request& req, std::string const &scriptPath);
        void    prepareEnv(Request &req);
        Request createTempHeader(std::string output);
        std::map<std::string, std::string> parseHeaders(const std::string& headerSection);
        std::pair<std::string, std::string> splitHeaderAndBody(const std::string& output);
        void    findBash();
        void    findPHP();
        void    findPython();
        void    findPl();
        void    cleanupResources();
        std::string getTimeStamp();
        std::string formatChunkedResponse(const std::string& body);
        bool    isChunkedTransfer(const std::map<std::string, std::string>& headers);
        // void    setNonBlocking(int fd);
    private:
        int                         _input[2];
        int                         _output[2];
        std::string                 _cgiBinPath;
        std::vector<std::string>    _supportedExt;
        std::vector<char*>          _env;
        char                        **_args;
        std::string                 _path;

        Client                      *_client;
        Server                      *_server;
        Config                      _config;
};

#endif