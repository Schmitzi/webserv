#include "../include/CGIHandler.hpp"
#include "../include/Client.hpp"
#include "../include/Helper.hpp"
#include "../include/ConfigValidator.hpp"
#include "../include/Server.hpp"
#include "../include/Webserv.hpp"
#include "../include/Response.hpp"
#include "../include/EpollHelper.hpp"
#include "../include/Response.hpp"

CGIHandler::CGIHandler(Client *client) {
	_client = client ? client : NULL;
	_server = client ? &client->getServer() : NULL;
	_input[0] = -1;
	_input[1] = -1;
	_output[0] = -1;
	_output[1] = -1;
	_cgiBinPath = "";
	_pathInfo = "";
	_path = "";
	_outputBuffer = "";
	_pid = -1;
	_startTime = 0;
	_timeout = TIMEOUT_SECONDS;
}

CGIHandler::CGIHandler(const CGIHandler& copy) {
	*this = copy;
}

CGIHandler& CGIHandler::operator=(const CGIHandler& copy) {
	if (this != &copy) {
		_client = copy._client;
		_server = copy._server;
		_input[0] = copy._input[0];
		_input[1] = copy._input[1];
		_output[0] = copy._output[0];
		_output[1] = copy._output[1];
		_cgiBinPath = copy._cgiBinPath;
		_pathInfo = copy._pathInfo;
		_env = copy._env;
		_args = copy._args;
		_path = copy._path;
		_outputBuffer = copy._outputBuffer;
		_pid = copy._pid;
		_req = copy._req;
		_startTime = copy._startTime;
		_timeout = copy._timeout;
	}
	return *this;
}


CGIHandler::~CGIHandler() {}

Client*  CGIHandler::getClient() const {
	return _client;
}

Request& CGIHandler::getRequest() {
	return _req;
}

void CGIHandler::setPath(const std::string& path) {
	_path = path;
}

void    CGIHandler::setCGIBin(serverLevel *config) {
	_cgiBinPath = "";
	
	std::map<std::string, locationLevel>::iterator it = config->locations.begin();
	for (; it != config->locations.end(); ++it) {
		if (iFind(it->first, "cgi-bin") != std::string::npos) {
			_cgiBinPath = it->second.cgiProcessorPath;
			break;
		}
	}
	
	if (_cgiBinPath.empty()) {
		_cgiBinPath = "/usr/bin/cgi-bin";
	}
}

int CGIHandler::executeCGI(Request &req) {
	_req = Request(req);
	if (doChecks() || prepareEnv()) {
		cleanupResources();
		return 1;
	}
	startClock();
	_pid = fork();
	if (_pid < 0) {
		_client->statusCode() = 500;
		_client->output() += getTimeStamp(_client->getFd()) + RED + "Error: fork() failed\n" + RESET;
		sendErrorResponse(*_client, _req);
		cleanupResources();
		return 1;
	}
	else if (_pid == 0)
		return doChild();
	return doParent();
}

int CGIHandler::doChecks() {
	if (access(_path.c_str(), F_OK) != 0) {
		_client->statusCode() = 404;
		_client->output() += getTimeStamp(_client->getFd()) + RED + "Error: Script does not exist: " + RESET + _path + "\n";
		sendErrorResponse(*_client, _req);
		return 1;
	}
	
	if (access(_path.c_str(), X_OK) != 0) {
		_client->statusCode() = 403;
		_client->output() += getTimeStamp(_client->getFd()) + RED + "Error: Script is not executable: " + RESET + _path + "\n";
		sendErrorResponse(*_client, _req);
		return 1;
	}
	
	if (pipe(_input) < 0 || pipe(_output) < 0) {
		_client->statusCode() = 500;
		_client->output() += getTimeStamp(_client->getFd()) + RED + "Error: pipe() failed" + RESET + "\n";
		sendErrorResponse(*_client, _req);
		return 1;
	}
	
	registerCgiPipe(_server->getWebServ(), _output[0], this);
	return 0;
}

int CGIHandler::prepareEnv() {
	std::string ext;
	std::string filePath;
	std::string fileName;
	std::string fileContent;
	locationLevel* loc = NULL;
	
	if (!_req.getQuery().empty())
		doQueryStuff(_req.getQuery(), fileName, fileContent);
	size_t dotPos = _path.find_last_of('/');
	if (dotPos != std::string::npos) {
		std::string temp = _path.substr(dotPos + 1);
		size_t slashPos = temp.find_first_of('/');
		if (slashPos != std::string::npos) {
			_pathInfo = temp.substr(slashPos);
			ext = "." + temp.substr(0, temp.find_first_of('/') - 1);
		} else
			ext = "." + temp;
		if (!matchLocation(ext, _req.getConf(), loc)) {
			_client->output() += getTimeStamp(_client->getFd()) + RED + "Error: Location not found for extension: " + RESET + ext + "\n";
			return 1;
		}
		if (loc->uploadDirPath.empty()) {
			_client->output() += getTimeStamp(_client->getFd()) + RED + "Error: Upload directory not set for extension: " + RESET + ext + "\n";
			return 1;
		}
		filePath = matchAndAppendPath(loc->uploadDirPath, fileName);
		makeArgs(loc->cgiProcessorPath, filePath);
	}
	
	_env.clear();
	
	_env.push_back("REDIRECT_STATUS=200");
	_env.push_back("SERVER_SOFTWARE=WebServ/1.0");
	_env.push_back("SERVER_NAME=" + _req.getConf().servName[0]);
	_env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	_env.push_back("SERVER_PROTOCOL=HTTP/1.1");
	_env.push_back("SERVER_PORT=" + tostring(_server->getConfParser().getPort(_req.getConf())));
	
	// Request information
	_env.push_back("REQUEST_METHOD=" + _req.getMethod());
	_env.push_back("QUERY_STRING=" + _req.getQuery());
	_env.push_back("CONTENT_TYPE=" + _req.getContentType());
	_env.push_back("CONTENT_LENGTH=" + tostring(_req.getBody().length()));
	
	// Script information
	_env.push_back("SCRIPT_NAME=" + _req.getPath());

	char cwd_buffer[1024];
	memset(cwd_buffer, 0, sizeof(cwd_buffer));

	if (getcwd(cwd_buffer, sizeof(cwd_buffer)) != NULL) {
		std::string current_dir(cwd_buffer);
		std::string script_filename = current_dir + "/" + _path;
		_env.push_back("SCRIPT_FILENAME=" + script_filename);
	} else {
		_env.push_back("SCRIPT_FILENAME=" + _path);
	}
	
	// PATH_INFO handling (for URLs like /script.php/extra/path)
	_env.push_back("PATH_INFO=" + _pathInfo);
	
	// Add HTTP headers as environment variables
	std::map<std::string, std::string> headers = _req.getHeaders();
	for (std::map<std::string, std::string>::iterator it = headers.begin(); 
		it != headers.end(); ++it) {
		std::string envName = "HTTP_" + it->first;
		std::transform(envName.begin(), envName.end(), envName.begin(), ::toupper);
		std::replace(envName.begin(), envName.end(), '-', '_');
		if (it->second == "keep-alive") 
			it->second = "close";
		_env.push_back(envName + "=" + it->second);
	}
	return 0;
}

void    CGIHandler::makeArgs(std::string const &cgiBin, std::string& filePath) {
	_args.clear();  
	_args.push_back(cgiBin);
	_args.push_back(_path);
	if (!filePath.empty())
		_args.push_back(filePath);
}

int CGIHandler::doChild() {
	close(_input[1]);
	close(_output[0]);
	
	if (dup2(_input[0], STDIN_FILENO) < 0 || dup2(_output[1], STDOUT_FILENO) < 0) {
		_client->statusCode() = 500;
		_client->output() += getTimeStamp(_client->getFd()) + RED + "Error: dup2() failed\n" + RESET;
		cleanupResources();
		return 1;
	}
	close(_input[0]);
	close(_output[1]);
	
	// Extract script directory and filename
	std::string scriptDir = _path;
	std::string scriptName = _path;
	size_t lastSlash = scriptDir.find_last_of('/');
	if (lastSlash != std::string::npos) {
		scriptDir = scriptDir.substr(0, lastSlash);
		scriptName = _path.substr(lastSlash + 1);

		if (chdir(scriptDir.c_str()) != 0) {
			_client->statusCode() = 500;
			_client->output() += getTimeStamp(_client->getFd()) + RED + "Error: chdir() failed to " + scriptDir + "\n" + RESET;
			cleanupResources();
			return 1;
		}
		
		for (size_t i = 0; i < _args.size(); i++) {
			if (_args[i] == _path) {
				_args[i] = scriptName;
				break;
			}
		}
	}
	
	std::vector<char*> argsPtrs;
	std::vector<char*> envPtrs;
	for (size_t i = 0; i < _args.size(); i++)
		argsPtrs.push_back(const_cast<char*>(_args[i].c_str()));
	argsPtrs.push_back(NULL);
	for (size_t i = 0; i < _env.size(); i++)
		envPtrs.push_back(const_cast<char*>(_env[i].c_str()));
	envPtrs.push_back(NULL);
	
	execve(argsPtrs[0], argsPtrs.data(), envPtrs.data());
	_client->statusCode() = 500;
	_client->output() += getTimeStamp(_client->getFd()) + RED + "Error: execve() failed\n" + RESET;
	cleanupResources();
	return 1;
}

void CGIHandler::prepareForExecve(std::vector<char*>& argsPtrs, std::vector<char*>& envPtrs) {
	for (size_t i = 0; i < _args.size(); i++)
		argsPtrs.push_back(const_cast<char*>(_args[i].c_str()));
	argsPtrs.push_back(NULL);
	for (size_t i = 0; i < _env.size(); i++)
		envPtrs.push_back(const_cast<char*>(_env[i].c_str()));
	envPtrs.push_back(NULL);
}

int CGIHandler::doParent() {
	close(_input[0]);
	close(_output[1]);

	if (!_req.getBody().empty()) {
		ssize_t w = write(_input[1], _req.getBody().c_str(), _req.getBody().length());
		if (!checkReturn(*_client, _client->getFd(), w, "write()")) {
			close(_input[1]);
			_input[1] = -1;
			_client->statusCode() = 500;
			sendErrorResponse(*_client, _req);
			cleanupResources();
			return 1;
		}
	}
	close(_input[1]);
	_input[1] = -1;

	if (addToEpoll(_server->getWebServ(), _output[0], EPOLLIN) != 0) {
		_client->statusCode() = 500;
		_client->output() += getTimeStamp(_client->getFd()) + RED + "Error: Failed to add CGI pipe to epoll\n" + RESET;
		sendErrorResponse(*_client, _req);
		cleanupResources();
		return 1;
	}
	return 0;
}

int CGIHandler::processScriptOutput() {
	char buffer[4096];
	int status;
	size_t maxSize = _outputBuffer.max_size();
	ssize_t bytesRead = -1;
	
	if (_outputBuffer.size() < maxSize) {
		bytesRead = read(_output[0], buffer, sizeof(buffer) - 1);
		if (!checkReturn(*_client, _client->getFd(), bytesRead, "read()")) {
			cleanupResources();
			if (_pid >= 0)
				kill(_pid, SIGKILL);
			_client->statusCode() = 500;
			sendErrorResponse(*_client, _req);
			return 1;
		} else if (bytesRead > 0){
			buffer[bytesRead] = '\0';
			_outputBuffer.append(buffer, bytesRead);
			return 0;
		}
	}
	pid_t result = waitpid(_pid, &status, WNOHANG);
	if (result == _pid) {
		_client->lastActive() = time(NULL);
		if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
			_client->output() += getTimeStamp(_client->getFd()) + GREEN + "CGI Script completed successfully\n" + RESET;
			
			if (_outputBuffer.empty()) {
				std::string defaultResponse = "HTTP/1.1 " + tostring(_client->statusCode()) + " " + getStatusMessage(_client->statusCode()) + "\r\n";
				defaultResponse += "Server: WebServ/1.0\r\n";
				defaultResponse += "Date: " + getCurrentTime() + "\r\n";
				defaultResponse += "Content-Type: text/plain\r\n";
				defaultResponse += "Content-Length: 22\r\n\r\n";
				defaultResponse += "No output from script\n";
				addSendBuf(_server->getWebServ(), _client->getFd(), defaultResponse);
				setEpollEvents(_server->getWebServ(), _client->getFd(), EPOLLOUT);
				cleanupResources();
				return 0;
			} else {
				std::pair<std::string, std::string> headerAndBody = splitHeaderAndBody(_outputBuffer);
				cleanupResources();
				if (_req.isChunkedTransfer())
					handleChunkedOutput(headerAndBody);
				else {
					if (_client->statusCode() == 501)
						return 1;
					handleStandardOutput(headerAndBody);
				}
				return 1;
			}
		} else {
			_client->output() += getTimeStamp(_client->getFd()) + RED + "Error: CGI Script exit status: " + RESET + tostring(WEXITSTATUS(status)) + "\n";
			_client->statusCode() = 502;
			sendErrorResponse(*_client, _req);
			cleanupResources();
			return 1;
		}
	} else if (result == -1) {
		_client->output() += getTimeStamp(_client->getFd()) + RED + "Error: waitpid() failed\n" + RESET;
		_client->statusCode() = 500;
		sendErrorResponse(*_client, _req);
		cleanupResources();
		return 1;
	}
	usleep(100000);
	return 0;
}

int CGIHandler::handleStandardOutput(const std::pair<std::string, std::string>& output) {
	int status = 200;
	if (output.first.find("Status:") != std::string::npos) {
		size_t statusPos = output.first.find("Status:") + 7;
		int statusCode = std::atoi(output.first.substr(statusPos).c_str());
		if (statusCode >= 100 && statusCode < 600)
			status = statusCode;
	}
	std::string contentType = "text/html";
	size_t contentTypePos = output.first.find("Content-Type:");
	if (contentTypePos != std::string::npos) {
		size_t endPos = output.first.find("\r\n", contentTypePos);
		if (endPos != std::string::npos) {
			contentType = output.first.substr(contentTypePos + 14, endPos - contentTypePos - 14);
			contentType.erase(std::remove(contentType.begin(), contentType.end(), ' '), contentType.end());
		}
	}
	std::string response = "HTTP/1.1 " + tostring(status) + " " + getStatusMessage(status) + "\r\n";
	response += "Server: WebServ/1.0\r\n";
	response += "Date: " + getCurrentTime() + "\r\n";
	response += "Content-Type: " + contentType + "\r\n";
	response += "Content-Length: " + tostring(output.second.length()) + "\r\n";
	if (shouldCloseConnection(_req))
		response += "Connection: close\r\n";
	else
		response += "Connection: keep-alive\r\n";
	response += "\r\n";
	response += output.second;
	addSendBuf(_server->getWebServ(), _client->getFd(), response);
	setEpollEvents(_server->getWebServ(), _client->getFd(), EPOLLOUT); 
	_client->lastActive() = time(NULL);   
	return 0;
}

int CGIHandler::handleChunkedOutput(const std::pair<std::string, std::string>& output) {
	int status = 200;
	if (output.first.find("Status:") != std::string::npos) {
		size_t statusPos = output.first.find("Status:") + 7;
		int statusCode = std::atoi(output.first.substr(statusPos).c_str());
		if (statusCode >= 100 && statusCode < 600)
			status = statusCode;
	}
	std::string response = "HTTP/1.1 " + tostring(status) + " " + getStatusMessage(status) + "\r\n";
	response += "Server: WebServ/1.0\r\n";
	response += "Date: " + getCurrentTime() + "\r\n";
	if (_req.isChunkedTransfer())
		response += "Transfer-Encoding: chunked\r\n";
	if (shouldCloseConnection(_req))
		response += "Connection: close\r\n";
	else
		response += "Connection: keep-alive\r\n";
	response += "\r\n";
	response += formatChunkedResponse(output.second);
	addSendBuf(_server->getWebServ(), _client->getFd(), response);
	setEpollEvents(_server->getWebServ(), _client->getFd(), EPOLLOUT);
	_client->lastActive() = time(NULL);
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

void	CGIHandler::startClock() {
	if (_startTime == 0)
		_startTime = time(NULL);
}

bool CGIHandler::isTimedOut(time_t now) const {
	if (_startTime == 0)
		return false;
	return (now - _startTime) > _timeout;
}

void CGIHandler::killProcess() {
	if (_pid > 0) {
		std::cerr << getTimeStamp(_client->getFd()) << RED << "Error: Killing CGI process " << _pid << RESET << std::endl;
		kill(_pid, SIGKILL);
		int status;
		waitpid(_pid, &status, WNOHANG);
		_pid = -1;
	}
}

void CGIHandler::cleanupResources() {
	if (!_client)
		return;

	if (_output[0] >= 0) {
		removeFromEpoll(_server->getWebServ(), _output[0]);
		if (isCgiPipeFd(_server->getWebServ(), _output[0]))
			unregisterCgiPipe(_server->getWebServ(), _output[0]);
		close(_output[0]);
		_output[0] = -1;
		_client->output() += getTimeStamp(_client->getFd()) + "Cleaned up and disconnected CGI\n";
	}
	_path.clear();
	_args.clear();
	_env.clear();
	_outputBuffer.clear();
	_startTime = 0;
	_client->statusCode() = 200;
	_client->shouldClose() = true;
}
