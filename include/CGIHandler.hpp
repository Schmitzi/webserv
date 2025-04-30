#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <iostream>
#include <vector>
#include <sys/wait.h>
#include <cstdlib>
// #include <fcntl.h>
#include <cerrno>
#include "Request.hpp"
#include "Helper.hpp"

class Client;

class CGIHandler {
    public:
        CGIHandler();
        ~CGIHandler();
        void    setClient(Client &client);
        bool    isCGIScript(const std::string& path);
        int     doChecks(Client client);
        int     processScriptOutput(Client &client);
        int     executeCGI(Client &client, Request& req, std::string const &scriptPath);
        void    prepareEnv(Request &req);
        Request createTempHeader(std::string output);
        void    cleanupResources();

        void    setPythonPath(char **envp);
    private:
        int                         _input[2];
        int                         _output[2];
        std::string                 _cgiBinPath;
        std::vector<std::string>    _supportedExt;
        std::vector<char*>          _env;
        char                        **_args;
        std::string                 _path;
        std::string                        _pythonPath;

        Client                      *_client;
};

#endif