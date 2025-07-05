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
		CGIHandler(const Client& client);
		CGIHandler(const CGIHandler& copy);
		CGIHandler&operator=(const CGIHandler& copy);
        ~CGIHandler();

        void									setServer(Server &server);
        void									setCGIBin(serverLevel *config);
		void									setPath(const std::string& path);
        std::string								getInfoPath();
        int										doChecks(Request& req);
        int										processScriptOutput();
        int										handleStandardOutput(const std::map<std::string, std::string>& headerMap, const std::string& initialBody);
        int										handleChunkedOutput(const std::map<std::string, std::string>& headerMap, const std::string& initialBody);
		void									prepareForExecve(std::vector<char*>& argsPtrs, std::vector<char*>& envPtrs);
		int										doChild();
		int										doParent(Request& req, int pid);
        int										executeCGI(Request& req);
		int										prepareEnv(Request &req);
        std::map<std::string, std::string>		parseHeaders(const std::string& headerSection);
        std::pair<std::string, std::string>		splitHeaderAndBody(const std::string& output);
		void									makeArgs(std::string const &cgiBin, std::string& filePath);
        void									cleanupResources();
        std::string								formatChunkedResponse(const std::string& body);
        bool									isChunkedTransfer(const std::map<std::string, std::string>& headers);
        std::string                             getInterpreterFromConfig(Request& req, const std::string& scriptPath);
        std::string                             getDirectoryFromPath(const std::string& fullPath);
        int                                     waitForCGICompletion(pid_t pid, int timeoutSeconds);
        std::string                             extractPathInfo(const std::string& requestPath, const std::string& scriptPath);
        Client*                                 getClient() const;

	private:
        int										_input[2];
        int										_output[2];
        std::string								_cgiBinPath;
        std::string								_pathInfo;
        std::vector<std::string>				_env;
		std::vector<std::string>				_args;
        std::string								_path;
        Client									*_client;
        Server									*_server;
		std::string								_outputBuffer;
};

#endif