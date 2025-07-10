#include "../include/CGIHandler.hpp"
#include "../include/Client.hpp"
#include "../include/Helper.hpp"
#include "../include/ConfigValidator.hpp"
#include "../include/Server.hpp"
#include "../include/Webserv.hpp"
#include "../include/Response.hpp"
#include "../include/EpollHelper.hpp"

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
	}
	return *this;
}

CGIHandler::~CGIHandler() {}

Client*  CGIHandler::getClient() const {
	return _client;
}

void CGIHandler::setServer(Server &server) {
	_server = &server;
}

void CGIHandler::setPath(const std::string& path) {
	_path = path;
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

int CGIHandler::executeCGI(Request &req) {
	_req = Request(req);
	if (doChecks() || prepareEnv())
		return 1;
	_pid = fork();
	if (_pid < 0) {
		std::cerr << getTimeStamp(_client->getFd()) << RED << "Error: fork() failed" << RESET << std::endl;
		sendErrorResponse(*_client, 500, _req);
		cleanupResources();
		return 1;
	}
	else if (_pid == 0)
		return doChild();
	return doParent();
}

int CGIHandler::doChecks() {
	if (access(_path.c_str(), F_OK) != 0) {
		std::cerr << getTimeStamp(_client->getFd()) << RED << "Script does not exist: " << RESET << _path << std::endl;
		sendErrorResponse(*_client, 404, _req);
		return 1;
	}
	
	if (access(_path.c_str(), X_OK) != 0) {
		std::cerr << getTimeStamp(_client->getFd()) << RED << "Script is not executable: " << RESET << _path << std::endl;
		sendErrorResponse(*_client, 403, _req);
		return 1;
	}
	
	if (pipe(_input) < 0 || pipe(_output) < 0) {
		std::cerr << getTimeStamp(_client->getFd()) << RED << "Error: pipe() failed" << RESET << std::endl;
		sendErrorResponse(*_client, 500, _req);
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
	
	if (!_req.getQuery().empty()) {
		doQueryStuff(_req.getQuery(), fileName, fileContent);
		size_t dotPos = _path.find_last_of('.');
		if (dotPos != std::string::npos) {
			ext = "." + _path.substr(dotPos + 1); 
			
			if (!matchLocation(ext, _req.getConf(), loc)) {
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
			
			if (!matchLocation(ext, _req.getConf(), loc)) {
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
	_env.push_back("SERVER_NAME=" + _req.getConf().servName[0]);
	_env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	_env.push_back("SERVER_PROTOCOL=HTTP/1.1");
	_env.push_back("SERVER_PORT=" + tostring(_server->getConfParser().getPort(_req.getConf())));
	_env.push_back("SERVER_PROTOCOL=HTTP/1.1");
	
	// Request information
	_env.push_back("REQUEST_METHOD=" + _req.getMethod());
	_env.push_back("QUERY_STRING=" + _req.getQuery());
	_env.push_back("CONTENT_TYPE=" + _req.getContentType());
	_env.push_back("CONTENT_LENGTH=" + tostring(_req.getBody().length()));
	
	// Script information
	_env.push_back("SCRIPT_NAME=" + _req.getPath());
	_env.push_back("SCRIPT_FILENAME=" + _path);
	
	// PATH_INFO handling (for URLs like /script.php/extra/path)
	_env.push_back("PATH_INFO=" + _pathInfo);
	
	// Add HTTP headers as environment variables
	std::map<std::string, std::string> headers = _req.getHeaders();
	for (std::map<std::string, std::string>::iterator it = headers.begin(); 
		it != headers.end(); ++it) {
		std::string envName = "HTTP_" + it->first;
		// Convert to uppercase and replace - with _
		std::transform(envName.begin(), envName.end(), envName.begin(), ::toupper);
		std::replace(envName.begin(), envName.end(), '-', '_');
		_env.push_back(envName + "=" + it->second);
	}
	_env.push_back("REDIRECT_STATUS=200");
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
	std::vector<char*> argsPtrs;
	std::vector<char*> envPtrs;
	prepareForExecve(argsPtrs, envPtrs);
	close(_input[1]);
	close(_output[0]);
	
	if (dup2(_input[0], STDIN_FILENO) < 0 || dup2(_output[1], STDOUT_FILENO) < 0) {
		std::cerr << getTimeStamp(_client->getFd()) << RED << "Error: dup2() failed" << RESET << std::endl;
		cleanupResources();
		return 1;
	}
	close(_input[0]);
	close(_output[1]);
	
	execve(argsPtrs[0], argsPtrs.data(), envPtrs.data());
	
	std::cerr << getTimeStamp(_client->getFd()) << RED << "Error: execve() failed" << RESET << std::endl;
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
		if (!checkReturn(_client->getFd(), w, "write()", "Nothing was written into _input[1]")) {
			close(_input[1]);
			_input[1] = -1;
			sendErrorResponse(*_client, 500, _req);
			cleanupResources();
			return 1;
		}
	}
	close(_input[1]);
	_input[1] = -1;

	if (addToEpoll(_server->getWebServ(), _output[0], EPOLLIN) != 0) {
		std::cerr << getTimeStamp(_client->getFd()) << RED << "Failed to add CGI pipe to epoll" << RESET << std::endl;
		sendErrorResponse(*_client, 500, _req);
		cleanupResources();
		return 1;
	}
	return 0;
}

int CGIHandler::processScriptOutput() {
	char buffer[4096];
	const int TIMEOUT_SECONDS = 20;
	static time_t startTime = time(NULL);
	int status;
	ssize_t bytesRead = -1;

	bytesRead = read(_output[0], buffer, sizeof(buffer) - 1);
	if (bytesRead > 0 && !(time(NULL) - startTime > TIMEOUT_SECONDS)) {
		buffer[bytesRead] = '\0';
		_outputBuffer.append(buffer, bytesRead);
		return 0;
	}
	while (true) {
		pid_t result = waitpid(_pid, &status, WNOHANG);
		if (result == _pid)
			break;
		else if (result == -1) {
			std::cerr << getTimeStamp(_client->getFd()) << RED << "Error: waitpid() failed" << RESET << std::endl;
			sendErrorResponse(*_client, 500, _req);
			cleanupResources();
			return 1;
		}
		if (time(NULL) - startTime > TIMEOUT_SECONDS) {
			std::cerr << getTimeStamp(_client->getFd()) << RED << "CGI timeout, killing process" << RESET << std::endl;
			cleanupResources();
			if (_pid >= 0)
				kill(_pid, SIGKILL);
			sendErrorResponse(*_client, 504, _req);
			return 1;
		}
		usleep(100000);
	}
	if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {// && bytesRead == 0) {
		std::cout << getTimeStamp(_client->getFd()) << GREEN << "CGI Script exit status: " << RESET << WEXITSTATUS(status) << std::endl;
		if (_outputBuffer.empty()) {
			std::string defaultResponse = "HTTP/1.1 200 OK\r\n";
			defaultResponse += "Content-Type: text/plain\r\n";
			defaultResponse += "Content-Length: 22\r\n\r\n";
			defaultResponse += "No output from script\n";

			addSendBuf(_server->getWebServ(), _client->getFd(), defaultResponse);
			setEpollEvents(_server->getWebServ(), _client->getFd(), EPOLLOUT);
			cleanupResources();
			return 0;
		} else {
			std::pair<std::string, std::string> headerAndBody = splitHeaderAndBody(_outputBuffer);
			std::map<std::string, std::string> headerMap = parseHeaders(headerAndBody.first);
			cleanupResources();
			if (isChunkedTransfer(headerMap))
				handleChunkedOutput(headerMap, headerAndBody.second);
			else
				handleStandardOutput(headerMap, headerAndBody.second);
			return 1;
		}
	} else {
		std::cerr << getTimeStamp(_client->getFd()) << RED << "CGI Script exit status: " << RESET << WEXITSTATUS(status) << std::endl;
		sendErrorResponse(*_client, 500, _req);
		cleanupResources();
		return 1;
	}
}

bool CGIHandler::isChunkedTransfer(const std::map<std::string, std::string>& headers) {
	std::map<std::string, std::string>::const_iterator it = headers.find("Transfer-Encoding");
	if (it != headers.end() && it->second.find("chunked") != std::string::npos)
		return true;
	return false;
}

int CGIHandler::handleStandardOutput(const std::map<std::string, std::string>& headerMap, const std::string& initialBody) {
	std::string response = "HTTP/1.1 200 OK\r\n";
	
	std::map<std::string, std::string>::const_iterator typeIt = headerMap.find("Content-Type");
	if (typeIt != headerMap.end())
		response += "Content-Type: " + typeIt->second + "\r\n";
	else
		response += "Content-Type: text/html\r\n";
	
	for (std::map<std::string, std::string>::const_iterator it = headerMap.begin(); it != headerMap.end(); ++it) {
		if (it->first != "Content-Type")
			response += it->first + ": " + it->second + "\r\n";
	}
	
	response += "Content-Length: " + tostring(initialBody.length()) + "\r\n";
	response += "Server: WebServ/1.0\r\n";
	response += "Connection: close\r\n";
	response += "\r\n";
	response += initialBody;
	
	_client->setConnect("close");
	addSendBuf(_server->getWebServ(), _client->getFd(), response);
	setEpollEvents(_server->getWebServ(), _client->getFd(), EPOLLOUT);    
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
	addSendBuf(_server->getWebServ(), _client->getFd(), response);
	setEpollEvents(_server->getWebServ(), _client->getFd(), EPOLLOUT);
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

void CGIHandler::cleanupResources() {
	if (!_client)
		return;
	for (int i = 0; i < 2; i++) {
		if (_input[i] >= 0) {
			close(_input[i]);
			_input[i] = -1;
		}
	}
	
	if (_output[1] >= 0) {
		close(_output[1]);
		_output[1] = -1;
	}
	
	if (_output[0] >= 0) {
		removeFromEpoll(_server->getWebServ(), _output[0]);
		if (isCgiPipeFd(_server->getWebServ(), _output[0]))
			unregisterCgiPipe(_server->getWebServ(), _output[0]);
		close(_output[0]);
		_output[0] = -1;
	}
	_path.clear();
	_args.clear();
	_env.clear();
	_outputBuffer.clear();
}