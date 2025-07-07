#include "../include/CGIHandler.hpp"
#include "../include/Client.hpp"

CGIHandler::CGIHandler(Webserv* webserv, Client* client, Server* server) {
	_client = client;
	_server = server;
	_webserv = webserv;
	_req = NULL;
	_input[0] = -1;
	_input[1] = -1;
	_output[0] = -1;
	_output[1] = -1;
	_pid = -1;
	_cgiBinPath = "";
	_pathInfo = "";
	_path = "";
	_inputDone = false;
	_outputDone = false;
	_errorDone = false;
	_inputOffset = 0;
}

CGIHandler::CGIHandler(const CGIHandler& copy) {
	*this = copy;
}

CGIHandler& CGIHandler::operator=(const CGIHandler& copy) {
	if (this != &copy) {
		_client = copy._client;
		_server = copy._server;
		_webserv = copy._webserv;
		_req = copy._req;
		_input[0] = copy._input[0];
		_input[1] = copy._input[1];
		_output[0] = copy._output[0];
		_output[1] = copy._output[1];
		_pid = copy._pid;
		_cgiBinPath = copy._cgiBinPath;
		_pathInfo = copy._pathInfo;
		_env = copy._env;
		_args = copy._args;
		_path = copy._path;
		_inputDone = copy._inputDone;
		_outputDone = copy._outputDone;
		_errorDone = copy._errorDone;
		_outputBuffer = copy._outputBuffer;
		_inputOffset = copy._inputOffset;
	}
	return *this;
}

CGIHandler::~CGIHandler() {
	// cleanupResources();//TODO: check
}

bool CGIHandler::inputIsDone() {
	return _inputDone;
}

bool CGIHandler::outputIsDone() {
	return _outputDone;
}

void    CGIHandler::setServer(Server &server) {
	_server = &server;
}

std::string CGIHandler::getInfoPath() {
	return _pathInfo;
}

void CGIHandler::setPath(const std::string& path) {
	_path = path;
}

void CGIHandler::printPipes() {
	std::cout << "_input[0]: " << _input[0] << " ";
	std::cout << "_input[1]: " << _input[1];
	std::cout << std::endl;
	std::cout << "_output[0]: " << _output[0] << " ";
	std::cout << "_output[1]: " << _output[1];
	std::cout << std::endl;
}

void CGIHandler::setReq(Request& req) {
	_req = &req;
}

void    CGIHandler::setCGIBin(serverLevel *config) {
	_cgiBinPath = "";
	
	std::map<std::string, locationLevel>::iterator it = config->locations.begin();
	for (; it != config->locations.end(); ++it) {
		if (it->first.find("cgi-bin") != std::string::npos) {
			_cgiBinPath = it->second.cgiProcessorPath;
			break;
		}
	}
	
	if (_cgiBinPath.empty()) {
		if (NIX == true)
			_cgiBinPath = "/etc/profiles/per-user/schmitzi/bin/php-cgi";
		else
			_cgiBinPath = "/usr/bin/cgi-bin";
	}
}

void CGIHandler::prepareForExecve(std::vector<char*>& argsPtrs, std::vector<char*>& envPtrs) {
	for (size_t i = 0; i < _args.size(); i++)
		argsPtrs.push_back(const_cast<char*>(_args[i].c_str()));
	argsPtrs.push_back(NULL);
	for (size_t i = 0; i < _env.size(); i++)
		envPtrs.push_back(const_cast<char*>(_env[i].c_str()));
	envPtrs.push_back(NULL);
}

int CGIHandler::doChild() {
	std::vector<char*> argsPtrs;
	std::vector<char*> envPtrs;
	prepareForExecve(argsPtrs, envPtrs);
	
	if (dup2(_input[0], STDIN_FILENO) < 0 || dup2(_output[1], STDOUT_FILENO) < 0) {
		std::cerr << getTimeStamp(_client->getFd()) << RED << "Error: dup2() failed" << RESET << std::endl;
		cleanupResources();
		return 1;
	}
	safeClose(_input[1]);
	safeClose(_output[0]);

	execve(argsPtrs[0], argsPtrs.data(), envPtrs.data());
	std::cerr << getTimeStamp(_client->getFd()) << RED << "Error: execve() failed: " << RESET << strerror(errno) << std::endl;
	_errorDone = true;
	cleanupResources();
	_client->sendErrorResponse(500, *_req);
	exit(1);
	return 1;
}

int CGIHandler::doParent() {
	safeClose(_input[0]);
	safeClose(_output[1]);

	if (_output[0] >= 0 && addToEpoll(*_webserv, _output[0], EPOLLIN) == 0)
		registerCgiPipe(*_webserv, _output[0], this);

	if (!_req->getBody().empty() && _input[1] >= 0) {
		if (addToEpoll(*_webserv, _input[1], EPOLLOUT) == 0)
			registerCgiPipe(*_webserv, _input[1], this);
	}

	return 0;
}

int CGIHandler::executeCGI() {
	if (doChecks() || prepareEnv()) {
		_errorDone = true;
		_client->sendErrorResponse(500, *_req);
		cleanupResources();
		return 1;
	}
	_pid = fork();
	if (_pid < 0) {
		std::cerr << getTimeStamp(_client->getFd()) << RED << "Error: fork() failed" << RESET << std::endl;
		_errorDone = true;
		_client->sendErrorResponse(500, *_req);
		cleanupResources();
		return 1;
	}
	else if (_pid == 0 && doChild() != 0)
		return 1;
	return doParent();
}

int CGIHandler::handleCgiPipeEvent(uint32_t events, int fd) {
	// std::cout << "\nhandle this fd: " << fd << std::endl;
	// _webserv->printCgis();

	if (!_inputDone || !_outputDone) {
		if ((events & EPOLLOUT) && !_inputDone) {
			if (!_req->getBody().empty() && fd == _input[1]) {
				ssize_t bytesWritten = -1;
				size_t remaining = _req->getBody().size() - _inputOffset;
				std::string s = _req->getBody().substr(_inputOffset);
				const char* data = s.c_str();

				if (!wrote(fd, data, remaining, bytesWritten, "Done with CGI input", false))
					return 1;
				_inputOffset += bytesWritten;
				if (_inputOffset >= _req->getBody().size()) {
					_inputDone = true;
					removeFromEpoll(*_webserv, fd);
					safeClose(fd);
					std::cout << MAGENTA << "CGI input done, closing pipe" << RESET << std::endl;
				}
			}
			return 0;
		}

		if ((events & EPOLLIN || events & EPOLLHUP) && !_outputDone) {
			if (fd == _output[0]) {
				char buffer[4096];
				ssize_t bytesRead = 0;

				if (!readIt(fd, buffer, sizeof(buffer) - 1, bytesRead, _outputBuffer, "Done with reading CGI output", false))
					return 0;

				if (bytesRead == 0) {
					_outputDone = true;
					removeFromEpoll(*_webserv, fd);
					safeClose(fd);
					std::cout << MAGENTA << "CGI output done, closing pipe" << RESET << std::endl;
					processScriptOutput();
				}
			}
			return 0;
		}
	}

	if (_inputDone && _outputDone && !_errorDone) {
		int status;
		pid_t result = waitpid(_pid, &status, WNOHANG);
		if (result == 0)
			return 0;
		else if (result == -1) {
			_errorDone = true;
			_client->sendErrorResponse(500, *_req);
			cleanupResources();
			return 1;
		}

		if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
			std::cout << getTimeStamp(_client->getFd()) << GREEN << "CGI Script exit status: " << RESET << "0" << std::endl;
		} else {
			_errorDone = true;
			std::cerr << getTimeStamp(_client->getFd()) << RED << "CGI Script failed" << RESET << std::endl;
			_client->sendErrorResponse(500, *_req);
		}

		cleanupResources();
		return 0;
	}

	return 0;
}

int CGIHandler::processScriptOutput() {
	if (_outputBuffer.empty()) {
		std::string defaultResponse = "HTTP/1.1 200 OK\r\n";
		defaultResponse += "Content-Type: text/plain\r\n";
		defaultResponse += "Content-Length: 22\r\n\r\n";
		defaultResponse += "No output from script\n";
		addSendBuf(*_webserv, _client->getFd(), defaultResponse);
		if (setEpollEvents(*_webserv, _client->getFd(), EPOLLOUT)) {
			std::cerr << getTimeStamp(_client->getFd()) << RED << "Error: setEpollEvents() failed 2" << RESET << std::endl;
			return 1;
		}
		return 0;
	}
	std::pair<std::string, std::string> headerAndBody = splitHeaderAndBody(_outputBuffer);
	std::map<std::string, std::string> headerMap = parseHeaders(headerAndBody.first);

	if (isChunkedTransfer(headerMap))
		return handleChunkedOutput(headerMap, headerAndBody.second);
	else
		return handleStandardOutput(headerMap, headerAndBody.second);
}


bool CGIHandler::isChunkedTransfer(const std::map<std::string, std::string>& headers) {
	std::map<std::string, std::string>::const_iterator it = headers.find("Transfer-Encoding");
	if (it != headers.end() && it->second.find("chunked") != std::string::npos)
		return true;
	return false;
}

int CGIHandler::handleStandardOutput(const std::map<std::string, std::string>& headerMap, const std::string& initialBody) {
	Request temp;
	
	std::map<std::string, std::string>::const_iterator typeIt = headerMap.find("Content-Type");
	if (typeIt != headerMap.end())
		temp.setContentType(typeIt->second);
	else
		temp.setContentType("text/html");
	
	temp.setBody(initialBody);
	_client->setCgiDone(true);
	_client->setConnect("keep-alive");
	_client->sendResponse(temp, "keep-alive", temp.getBody());
	return 0;
}

int CGIHandler::handleChunkedOutput(const std::map<std::string, std::string>& headerMap, const std::string& initialBody) {
	std::string response = "HTTP/1.1 200 OK\r\n";
	for (std::map<std::string, std::string>::const_iterator it = headerMap.begin(); it != headerMap.end(); ++it) {
		if (it->first != "Content-Length")
			response += it->first + ": " + it->second + "\r\n";
	}
	
	if (headerMap.find("Transfer-Encoding") == headerMap.end())
		response += "Transfer-Encoding: chunked\r\n";
	
	response += "Connection: keep-alive\r\n";
	response += "\r\n";
	_client->setConnect("keep-alive");
	response += formatChunkedResponse(initialBody);
	addSendBuf(*_webserv, _client->getFd(), response);
	_client->setCgiDone(true);
	if (setEpollEvents(*_webserv, _client->getFd(), EPOLLOUT)) {
		std::cerr << getTimeStamp(_client->getFd()) << RED << "Error: setEpollEvents() failed 3" << RESET << std::endl;
		cleanupResources();
		return 1;
	}
	return 0;
}

std::string CGIHandler::formatChunkedResponse(const std::string& body) {
	std::string chunkedBody = "";
	
	if (body.empty()) {
		chunkedBody = "0\r\n\r\n";
		return chunkedBody;
	}
	
	const size_t chunkSize = 4096;
	size_t remaining = body.length();
	size_t offset = 0;
	
	while (remaining > 0) {
		size_t currentChunkSize = (remaining < chunkSize) ? remaining : chunkSize;
		
		std::stringstream hexStream;
		hexStream << std::hex << currentChunkSize;
		chunkedBody += hexStream.str() + "\r\n";
		
		chunkedBody += body.substr(offset, currentChunkSize) + "\r\n";
		
		offset += currentChunkSize;
		remaining -= currentChunkSize;
	}
	
	chunkedBody += "0\r\n\r\n";
	
	return chunkedBody;
}

std::pair<std::string, std::string> CGIHandler::splitHeaderAndBody(const std::string& output) {
	size_t headerEnd = output.find("\r\n\r\n");
	if (headerEnd == std::string::npos) {
		headerEnd = output.find("\n\n");
		if (headerEnd == std::string::npos) {
			return std::make_pair("", output);
		} else {
			return std::make_pair(
				output.substr(0, headerEnd),
				output.substr(headerEnd + 2)
			);
		}
	} else {
		return std::make_pair(
			output.substr(0, headerEnd),
			output.substr(headerEnd + 4)
		);
	}
}

std::map<std::string, std::string> CGIHandler::parseHeaders(const std::string& headerSection) {
	std::map<std::string, std::string> headers;
	std::istringstream iss(headerSection);
	std::string line;
	
	while (std::getline(iss, line) && !line.empty() && line != "\r") {
		if (!line.empty() && line[line.length() - 1] == '\r') {
			line.erase(line.length() - 1);
		}
		
		size_t colonPos = line.find(':');
		if (colonPos != std::string::npos) {
			std::string key = line.substr(0, colonPos);
			std::string value = line.substr(colonPos + 1);
			
			key.erase(0, key.find_first_not_of(" \t"));
			key.erase(key.find_last_not_of(" \t") + 1);
			value.erase(0, value.find_first_not_of(" \t"));
			value.erase(value.find_last_not_of(" \t") + 1);
			
			headers[key] = value;
		}
	}
	return headers;
}

int CGIHandler::prepareEnv() {
	std::string ext;
	std::string filePath;
	std::string fileName;
	std::string fileContent;
	locationLevel* loc = NULL;
	
	if (!_req->getQuery().empty()) {
		doQueryStuff(_req->getQuery(), fileName, fileContent);
		size_t dotPos = _path.find_last_of('.');
		if (dotPos != std::string::npos) {
			ext = "." + _path.substr(dotPos + 1); 
			
			if (!matchLocation(ext, _req->getConf(), loc)) {
				std::cerr << getTimeStamp(_client->getFd()) << RED << "Location not found for extension: " << RESET << ext << std::endl;
				return 1;
			}
			
			if (loc->uploadDirPath.empty()) {
				std::cerr << getTimeStamp(_client->getFd()) << RED << "Upload directory not set for extension: " << RESET << ext << std::endl;
				return 1;
			}
			filePath = matchAndAppendPath(loc->uploadDirPath, fileName);
			
			makeArgs(loc->cgiProcessorPath, filePath);
		}
	} else {
		size_t dotPos = _path.find_last_of('.');
		if (dotPos != std::string::npos) {
			ext = "." + _path.substr(dotPos + 1);
			
			if (!matchLocation(ext, _req->getConf(), loc)) {
				std::cerr << getTimeStamp(_client->getFd()) << RED << "Location not found for extension: " << RESET << ext << std::endl;
				return 1;
			}
			makeArgs(loc->cgiProcessorPath, filePath);
		}
	}
	
	_env.clear();
	
	const std::string abs_path = matchAndAppendPath(loc->rootLoc, _path);
	_env.push_back("SCRIPT_FILENAME=" + abs_path);
	_env.push_back("REDIRECT_STATUS=200");
	_env.push_back("SERVER_SOFTWARE=WebServ/1.0");
	_env.push_back("SERVER_NAME=" + _req->getConf().servName[0]);
	_env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	_env.push_back("SERVER_PROTOCOL=HTTP/1.1");
	_env.push_back("SERVER_PORT=" + tostring(_server->getConfParser().getPort(_req->getConf())));
	_env.push_back("REQUEST_METHOD=" + _req->getMethod());
	_env.push_back("PATH_INFO=" + getInfoPath());
	
	size_t slashPos = _path.find_last_of('/');
	std::string script;
	if (slashPos != std::string::npos) {
		script = _path.substr(slashPos + 1);
		_env.push_back("SCRIPT_NAME=/" + script);
	}
	_env.push_back("QUERY_STRING=" + _req->getQuery());
	_env.push_back("CONTENT_TYPE=" + _req->getContentType());
	_env.push_back("CONTENT_LENGTH=" + tostring(_req->getBody().length()));
	
	return 0;
}

void    CGIHandler::makeArgs(std::string const &cgiBin, std::string& filePath) {
	_args.clear();  
	_args.push_back(cgiBin);
	_args.push_back(_path);
	if (!filePath.empty())
		_args.push_back(filePath);
}

void CGIHandler::cleanupResources() {
	if (_webserv) {
		if (_input[1] >= 0) unregisterCgiPipe(*_webserv, _input[1]);
		if (_output[0] >= 0) unregisterCgiPipe(*_webserv, _output[0]);
	}
	safeClose(_input[0]);
	safeClose(_input[1]);
	safeClose(_output[0]);
	safeClose(_output[1]);
}

int CGIHandler::doChecks() {
	if (access(_path.c_str(), F_OK) != 0) {
		std::cerr << getTimeStamp(_client->getFd()) << RED << "Error: Script does not exist: " << RESET << _path << std::endl;
		_client->sendErrorResponse(404, *_req);
		cleanupResources();
		return 1;
	}

	if (access(_path.c_str(), X_OK) != 0) {
		std::cerr << getTimeStamp(_client->getFd()) << RED << "Error: Script is not executable: " << RESET << _path << std::endl;
		_client->sendErrorResponse(403, *_req);
		cleanupResources();
		return 1;
	}

	if (pipe(_input) < 0 || pipe(_output) < 0) {
		std::cerr << getTimeStamp(_client->getFd()) << RED << "Error: Pipe creation failed" << RESET << std::endl;
		_client->sendErrorResponse(500, *_req);
		cleanupResources();
		return 1;
	}
	return 0;
}