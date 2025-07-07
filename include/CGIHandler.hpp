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
#include "NoErrNo.hpp"

class	Client;
class	Server;
class	Request;
class	Webserv;
struct	serverLevel;

#define NIX false

class CGIHandler {
	public:
		CGIHandler();
		CGIHandler(Webserv* webserv, Client* client, Server* server, Request* request);
		CGIHandler(const CGIHandler& copy);
		CGIHandler&operator=(const CGIHandler& copy);
		~CGIHandler();

		void									setServer(Server &server);
		void									setCGIBin(serverLevel *config);
		void									setPath(const std::string& path);
		std::string								getInfoPath();
		int										doChecks(Request& req);
		int										handleCgiPipeEvent(uint32_t events, int fd);
		int										processScriptOutput();
		int										handleStandardOutput(const std::map<std::string, std::string>& headerMap, const std::string& initialBody);
		int										handleChunkedOutput(const std::map<std::string, std::string>& headerMap, const std::string& initialBody);
		void									prepareForExecve(std::vector<char*>& argsPtrs, std::vector<char*>& envPtrs);
		int										doChild();
		int										doParent();
		int										executeCGI(Request& req);
		int										prepareEnv(Request &req);
		std::map<std::string, std::string>		parseHeaders(const std::string& headerSection);
		std::pair<std::string, std::string>		splitHeaderAndBody(const std::string& output);
		void									makeArgs(std::string const &cgiBin, std::string& filePath);
		void									cleanupResources();
		std::string								formatChunkedResponse(const std::string& body);
		bool									isChunkedTransfer(const std::map<std::string, std::string>& headers);
		bool									inputIsDone();
		bool									outputIsDone();
		void									printPipes();

	private:
		int										_input[2];
		int										_output[2];
		pid_t									_pid;
		std::string								_cgiBinPath;
		std::string								_pathInfo;
		std::vector<std::string>				_env;
		std::vector<std::string>				_args;
		std::string								_path;
		Client									*_client;
		Server									*_server;
		Request									*_request;
		Webserv									*_webserv;
		std::string								_outputBuffer;
		bool									_inputDone;
		bool									_outputDone;
		bool									_errorDone;
		size_t									_inputOffset;
};

#endif