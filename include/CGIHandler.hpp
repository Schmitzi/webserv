#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <iostream>
#include <vector>
#include <sys/wait.h>
#include <cstdlib>
#include <map>
#include "Request.hpp"

class	Client;
class	Server;
struct	serverLevel;

#define NIX false

class CGIHandler {
	public:
		CGIHandler(Client *client = NULL);
		CGIHandler(const CGIHandler& copy);
		CGIHandler&operator=(const CGIHandler& copy);
		~CGIHandler();

		//getters & setters
		Client*									getClient() const;
		void									setServer(Server &server);
		void									setPath(const std::string& path);
		void									setCGIBin(serverLevel *config);

		int										executeCGI(Request& req);
		int										doChecks();
		int										prepareEnv();
		void									makeArgs(std::string const &cgiBin, std::string& filePath);
		int										doChild();
		void									prepareForExecve(std::vector<char*>& argsPtrs, std::vector<char*>& envPtrs);
		int										doParent();
		int										processScriptOutput();
		int										handleStandardOutput(const std::string& initialBody);
		int										handleChunkedOutput(const std::string& initialBody);
		std::string								formatChunkedResponse(const std::string& body);
		std::pair<std::string, std::string>		splitHeaderAndBody(const std::string& output);
		void									cleanupResources();

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
		pid_t									_pid;
		Request									_req;
		time_t									_startTime;
};

#endif