#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <iostream>
#include <vector>
#include <sys/wait.h>
#include <cstdlib>
#include <cerrno>
#include "Request.hpp"
#include "Helper.hpp"
#include "ConfigValidator.hpp"

class	Client;
class	Server;
class	Request;
struct	serverLevel;

#define NIX false

class CGIHandler {
    public:
		CGIHandler();
        ~CGIHandler();

        void									setClient(Client &client);
        void									setServer(Server &server);
        void									setCGIBin(serverLevel *config);
        std::string								getInfoPath();
		void									setPathInfo(Request& req);
        bool									isCGIScript(const std::string& path);
        int										doChecks(Client client, Request& req);
        int										processScriptOutput(Client &client);
        int										handleStandardOutput(const std::map<std::string, std::string>& headerMap, const std::string& initialBody);
        int										handleChunkedOutput(const std::map<std::string, std::string>& headerMap, const std::string& initialBody);
        int										executeCGI(Client &client, Request& req, std::string const &scriptPath);
        int										prepareEnv(Request &req);
        std::map<std::string, std::string>		parseHeaders(const std::string& headerSection);
        std::pair<std::string, std::string>		splitHeaderAndBody(const std::string& output);
		void									makeArgs(std::string const &cgiBin, std::string& filePath);
        void									cleanupResources();
        std::string								getTimeStamp();
        std::string								formatChunkedResponse(const std::string& body);
        bool									isChunkedTransfer(const std::map<std::string, std::string>& headers);

	private:
        int										_input[2];
        int										_output[2];
        std::string								_cgiBinPath;
        std::vector<std::string>				_supportedExt;
        std::string								_pathInfo;
        std::vector<std::string>				_env;
		std::vector<std::string>				_args;
        std::string								_path;
        Client									*_client;
        Server									*_server;
};

#endif