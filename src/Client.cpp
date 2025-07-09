#include "../include/Client.hpp"
#include "../include/Server.hpp"
#include "../include/Webserv.hpp"
#include "../include/Response.hpp"
#include "../include/Helper.hpp"
#include "../include/Request.hpp"
#include "../include/CGIHandler.hpp"
#include "../include/Multipart.hpp"
#include "../include/ConfigParser.hpp"
#include "../include/EpollHelper.hpp"
#include "../include/ClientHelper.hpp"

Client::Client(Server& serv) {
	_addr = serv.getAddr();
	_fd = serv.getFd();
	_webserv = &serv.getWebServ();
	_server = &serv;
	_configs = serv.getConfigs();
	_connect = "";
	_sendOffset = 0;
	_exitCode = 0;
}

Client::Client(const Client& client) {
	*this = client;
}

Client& Client::operator=(const Client& other) {
	if (this != &other) {
		_addr = other._addr;
		_fd = other._fd;
		_addrLen = other._addrLen;
		_requestBuffer = other._requestBuffer;
		_webserv = other._webserv;
		_server = other._server;
		_configs = other._configs;
		_connect = other._connect;
		_sendOffset = other._sendOffset;
		_exitCode = other._exitCode;
	}
	return *this;
}

Client::~Client() {}

int	&Client::getFd() {
	return _fd;
}

Server &Client::getServer() {
	return *_server;
}

Webserv& Client::getWebserv() {
	return *_webserv;
}

size_t& Client::getOffset() {
	return _sendOffset;
}

int Client::getExitCode() {
	return _exitCode;
}

std::vector<serverLevel> Client::getConfigs() {
	return _configs;
}

void Client::setConnect(std::string connect) {
	_connect = connect;
}

void Client::setExitCode(int i) {
	_exitCode = i;
}

int Client::acceptConnection(int serverFd) {
	_addrLen = sizeof(_addr);
	_fd  = accept(serverFd, (struct sockaddr *)&_addr, &_addrLen);
	if (_fd < 0) {
		std::cerr << getTimeStamp() << RED << "Error: accept() failed" << RESET << std::endl;
		return 1;
	}
	std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
	std::cout << getTimeStamp(_fd) << "Client accepted" << std::endl;
	
	return 0;
}

void Client::displayConnection() {
	unsigned char *ip = (unsigned char *)&_addr.sin_addr.s_addr;
	std::cout << getTimeStamp(_fd) << BLUE << "New connection from "
		<< (int)ip[0] << "." << (int)ip[1] << "." << (int)ip[2] << "."
			<< (int)ip[3] << ":" << ntohs(_addr.sin_port) << RESET << std::endl;
}

void Client::recieveData() {
	static bool printNewLine = false;
	char buffer[1000000];
	memset(buffer, 0, sizeof(buffer));
	
	ssize_t bytesRead = recv(_fd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
	if (bytesRead < 0) {
		_exitCode = 1;
		std::cerr << getTimeStamp(_fd) << RED << "Error: recv() failed" << RESET << std::endl;
		return;
	}
	
	if (bytesRead == 0) {
		_exitCode = 1;
		return;
	}
	
	_requestBuffer.append(buffer, bytesRead);
	bool isChunked = (_requestBuffer.find("Transfer-Encoding:") != std::string::npos &&
						_requestBuffer.find("chunked") != std::string::npos);
	
	if (isChunked) {
		if (!isChunkedBodyComplete(_requestBuffer)) {
			std::cout << getTimeStamp(_fd) << BLUE 
						<< "Chunked body incomplete, waiting for more data" << RESET << std::endl;
			_exitCode = 0;
			return;
		}
	} else {
		if (checkLength(_requestBuffer, _fd, printNewLine) == 0) {
			_exitCode = 0;
			return;
		}
	}
	if (printNewLine == true)
		std::cout << std::endl;
	std::cout << getTimeStamp(_fd) << GREEN  << "Complete request received, processing..." << RESET << std::endl;
	printNewLine = false;
	
	if (_requestBuffer.empty() || _requestBuffer.find("HTTP/") == std::string::npos) {
		std::cout << getTimeStamp(_fd) << YELLOW << "Empty or invalid request received, closing connection" << RESET << std::endl;
		_requestBuffer.clear();
		_exitCode = 1;
		return;
	}
	
	Request req(_requestBuffer, *this, _fd);
	_exitCode = processRequest(req);
	if (_exitCode != 1)
		_requestBuffer.clear();
}

int Client::processRequest(Request& req) {
	serverLevel &conf = req.getConf();
	if (req.getContentLength() > conf.requestLimit) {
		std::cerr << getTimeStamp(_fd) << RED  << "Content-Length too large" << RESET << std::endl;
		sendErrorResponse(*this, 413, req);
		_requestBuffer.clear();
		return 1;
	}

	if (req.getCheck() == "BAD") {
		std::cerr << getTimeStamp(_fd) << RED  << "Bad request format " << RESET << std::endl;
		sendErrorResponse(*this, 400, req);
		_requestBuffer.clear();
		return 1;
	}
	
	if (req.getCheck() == "NOTALLOWED") {
		std::cerr << getTimeStamp(_fd) << RED  << "Method Not Allowed: " << RESET << req.getMethod() << std::endl;
		sendErrorResponse(*this, 405, req);
		_requestBuffer.clear();
		return 1;
	}

	locationLevel* loc = NULL;
	if (matchLocation(req.getPath(), conf, loc)) {
		if (loc->hasRedirect == true) {
			sendRedirect(*this, loc->redirectionHTTP.first, loc->redirectionHTTP.second);
			return 0;
		}
	}
	if (req.getContentType().find("multipart/form-data") != std::string::npos) {
		int result = handleMultipartPost(req);
		if (result != -1)
			return result;
		return 0;
	}
	std::cout << getTimeStamp(_fd) << BLUE << "Parsed Request: " << RESET << 
		req.getMethod() << " " << req.getPath() << " " << req.getVersion() << std::endl;

	if (req.getMethod() == "GET")
		return handleGetRequest(req);
	else if (req.getMethod() == "POST")
		return handlePostRequest(req);
	else if (req.getMethod() == "DELETE")
		return handleDeleteRequest(req);
	else {
		sendErrorResponse(*this, 405, req);
		return 1;
	}
}

int Client::handleGetRequest(Request& req) {
	std::string requestPath = req.getPath();
	locationLevel* loc = NULL;
	if (!matchLocation(req.getPath(), req.getConf(), loc)) {
		std::cerr << getTimeStamp(_fd) << RED  << "Location not found: " << RESET << req.getPath() << std::endl;
		sendErrorResponse(*this, 404, req);
		return 1;
	}
	if (requestPath.find("../") != std::string::npos) {
		sendErrorResponse(*this, 403, req);
		return 1;
	}
	if (loc->autoindex == true && isFileBrowserRequest(requestPath))
		return handleFileBrowserRequest(req);
	else
		return handleRegularRequest(req);
}

int Client::handlePostRequest(Request& req) {
	locationLevel* loc = NULL;
	if (!matchLocation(req.getPath(), req.getConf(), loc)) {
		std::cerr << getTimeStamp(_fd) << RED  << "Location not found for POST request: " 
				<< RESET << req.getPath() << std::endl;
		sendErrorResponse(*this, 404, req);
		return 1;
	}
	std::string fullPath = getLocationPath(*this, req, "POST");
	if (fullPath.empty())
		return 1;
	if (isCGIScript(req.getPath())) {
		CGIHandler* cgi = new CGIHandler(this);
		std::string cgiPath = matchAndAppendPath(_server->getWebRoot(req, *loc), req.getPath());
		cgi->setPath(cgiPath);
		int result = cgi->executeCGI(req);
		if (result != 0)
			delete cgi;
		return result;
	}
	
	if (req.getContentType().find("multipart/form-data") != std::string::npos)
		return handleMultipartPost(req);
	
	if (req.getPath().find("../") != std::string::npos) {
		sendErrorResponse(*this, 403, req);
		return 1;
	}
	
	std::string contentToWrite;
	
	if (isChunkedRequest(req)) {
		std::cout << getTimeStamp(_fd) << BLUE << "Processing chunked request" << RESET << std::endl;
		contentToWrite = decodeChunkedBody(_fd, req.getBody());
		
		if (contentToWrite.empty()) {
			std::cerr << getTimeStamp(_fd) << RED  << "Failed to decode chunked data" << RESET << std::endl;
			sendErrorResponse(*this, 400, req);
			return 1;
		}
	} else {
		contentToWrite = req.getBody();
		std::string fileName;
		if (contentToWrite.empty() && !req.getQuery().empty())
			doQueryStuff(req.getQuery(), fileName, contentToWrite);
		else
			doQueryStuff(req.getBody(), fileName, contentToWrite);
		fullPath = matchAndAppendPath(fullPath, fileName);
	}
	int fd = open(fullPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		std::cerr << getTimeStamp(_fd) << RED << "Failed to open file for writing: " << RESET << fullPath << std::endl;
		sendErrorResponse(*this, 500, req);
		return 1;
	}
	
	std::cout << getTimeStamp(_fd) << BLUE << "Writing to file: " << RESET << fullPath << std::endl;
	
	ssize_t bytesWritten = write(fd, contentToWrite.c_str(), contentToWrite.length());
	
	if (!checkReturn(_fd, bytesWritten, "write()", "Failed to write to file")) {
		sendErrorResponse(*this, 500, req);
		close(fd);
		return 1;
	}
	std::string responseBody = "File uploaded successfully. Wrote " + tostring(bytesWritten) + " bytes.";
	_connect = "keep-alive";
	ssize_t responseResult = sendResponse(*this, req, "keep-alive", responseBody);
	
	if (responseResult < 0) {
		std::cerr << getTimeStamp(_fd) << RED  << "Failed to send response" << RESET << std::endl;
		return 1;
	}
	
	std::cout << getTimeStamp(_fd) << GREEN  << "Uploaded file: " << RESET << fullPath 
			<< " (" << bytesWritten << " bytes written)" << std::endl;
	close(fd);
	return 0;
}

int Client::handleDeleteRequest(Request& req) {
	std::string fullPath = getLocationPath(*this, req, "DELETE");
	if (fullPath.empty())
		return 1;
	if (isCGIScript(req.getPath())) {
		CGIHandler* cgi = new CGIHandler(this);
		cgi->setPath(fullPath);        
		int result = cgi->executeCGI(req);
		if (result != 0)
			delete cgi;
		return result;
	}

	size_t end = fullPath.find_last_not_of(" \t\r\n");
	if (end != std::string::npos)
		fullPath = fullPath.substr(0, end + 1);

	if (unlink(fullPath.c_str()) != 0) {
		if (errno == ENOENT) {
			std::cerr << getTimeStamp(_fd) << RED  << "File not found for deletion: " << RESET << fullPath << std::endl;
			sendErrorResponse(*this, 404, req);
			return 1;
		} else if (errno == EACCES || errno == EPERM) {
			std::cerr << getTimeStamp(_fd) << RED  << "Permission denied for deletion: " << RESET << fullPath << std::endl;
			sendErrorResponse(*this, 403, req);
			return 1;
		} else {
			std::cerr << getTimeStamp(_fd) << RED  << "Error deleting file: " << RESET << fullPath
					<< " - " << strerror(errno) << std::endl;
			sendErrorResponse(*this, 500, req);
			return 1;
		}
	}
	sendResponse(*this, req, "keep-alive", "");
	return 0;
}

int Client::handleFileBrowserRequest(Request& req) {
	std::string requestPath = req.getPath();
	std::string actualPath;
	if (requestPath == "/root" || requestPath == "/root/")
		actualPath = "/";
	else if (requestPath.find("/root/") == 0) {
		actualPath = requestPath.substr(5);
		if (actualPath.empty()) actualPath = "/";
	} else {
		locationLevel* loc = NULL;
		matchLocation(requestPath, req.getConf(), loc);
		actualPath = requestPath.substr(5);
		std::string actualFullPath = matchAndAppendPath(_server->getWebRoot(req, *loc), actualPath);
		
		struct stat fileStat;
		if (stat(actualFullPath.c_str(), &fileStat) != 0) {
			std::cerr << getTimeStamp(_fd) << RED  << "File not found: " << RESET << actualFullPath  << std::endl;
			sendErrorResponse(*this, 404, req);
			return 1;
		}
		if (S_ISDIR(fileStat.st_mode)) {
			req.setPath(actualPath);
			return createDirList(actualFullPath, req);
		} else if (S_ISREG(fileStat.st_mode)) {
			if (buildBody(*this, req, actualFullPath) == 1)
				return 1;
			req.setContentType(req.getMimeType(actualFullPath));
			sendResponse(*this, req, "keep-alive", req.getBody());
			std::cout << getTimeStamp(_fd) << GREEN  << "Successfully served file from browser: " 
					<< RESET << actualFullPath << std::endl;
			return 0;
		} else {
			std::cerr << getTimeStamp(_fd) << RED << "Not a regular file or directory: " << RESET << actualFullPath << std::endl;
			sendErrorResponse(*this, 403, req);
			return 1;
		}
	}
	return 0;
}

int Client::handleRegularRequest(Request& req) {
	locationLevel* loc = NULL;
	if (!matchLocation(req.getPath(), req.getConf(), loc)) {
		std::cerr << getTimeStamp(_fd) << RED  << "Location not found: " << RESET << req.getPath() << std::endl;
		sendErrorResponse(*this, 404, req);
		return 1;
	}
	std::string reqPath = req.getPath();
	if (reqPath == "/" || reqPath.empty())
		reqPath = loc->indexFile;

	size_t end = reqPath.find_last_not_of(" \t\r\n");
	if (end != std::string::npos)
		reqPath = reqPath.substr(0, end + 1);
	std::string fullPath;
	if (reqPath.find("/home") == std::string::npos)
		fullPath = matchAndAppendPath(_server->getWebRoot(req, *loc), reqPath);
	else
		fullPath = reqPath;

	if (fullPath.find("root") != std::string::npos && loc->autoindex == false) {
		std::cerr << getTimeStamp(_fd) << RED << "Access to directory browser is forbidden when autoindex is off" << RESET << std::endl;
		sendErrorResponse(*this, 403, req);
		return 1;
	}

	if (handleRedirect(req) == 0)
		return 1;
		
	if (isCGIScript(reqPath)) {      
		CGIHandler* cgi = new CGIHandler(this);
		cgi->setCGIBin(&req.getConf());
		std::string fullCgiPath = matchAndAppendPath(_server->getWebRoot(req, *loc), reqPath);
		cgi->setPath(fullCgiPath);
		int result = cgi->executeCGI(req);
		if (result != 0)
			delete cgi;
		return result;
	}

	std::cout << getTimeStamp(_fd) << BLUE << "Handling GET request for path: " << RESET << req.getPath() << std::endl;

	struct stat fileStat;
	if (stat(fullPath.c_str(), &fileStat) != 0) {
		std::cerr << getTimeStamp(_fd) << RED  << "File not found: " << RESET << fullPath << std::endl;
		sendErrorResponse(*this, 404, req);
		return 1;
	}
	
	if (S_ISDIR(fileStat.st_mode))
		return viewDirectory(fullPath, req);
	else if (!S_ISREG(fileStat.st_mode)) {
		std::cerr << getTimeStamp(_fd) << RED << "Not a regular file: " << RESET << fullPath << std::endl;
		sendErrorResponse(*this, 403, req);
		return 1;
	}
	
	if (buildBody(*this, req, fullPath) == 1)
		return 1;

	std::string contentType = req.getMimeType(fullPath);
	if (fullPath.find(".html") != std::string::npos || reqPath == "/" || reqPath == loc->indexFile)
		contentType = "text/html";

	req.setContentType(contentType);

	sendResponse(*this, req, "close", req.getBody());
	std::cout << getTimeStamp(_fd) << GREEN  << "Sent file: " << RESET << fullPath << std::endl;
	return 0;
}

int Client::handleMultipartPost(Request& req) {
	std::string boundary = req.getBoundary();
	
	if (boundary.empty()) {
		sendErrorResponse(*this, 400, req);
		return 1;
	}
	
	Multipart parser(_requestBuffer, boundary);
	
	if (!parser.parse()) {
		sendErrorResponse(*this, 400, req);
		return 1;
	}
	
	std::string filename = parser.getFilename();
	if (filename.empty()) {
		sendErrorResponse(*this, 400, req);
		return 1;
	}

	std::string fileContent = parser.getFileContent();
	if (fileContent.empty() && !parser.isComplete()) {
		return 1;
	}
	
	if (!saveFile(req, filename, fileContent)) {
		sendErrorResponse(*this, 500, req);
		return 1;
	}
	std::cout << getTimeStamp(_fd) << GREEN  << "Received: " + parser.getFilename() << RESET << std::endl;
	std::string successMsg = "Successfully uploaded file:" + filename;
	sendResponse(*this, req, "close", successMsg);
	std::cout << getTimeStamp(_fd) << GREEN  << "File transfer ended" << RESET << std::endl;    
	return 0;
}

int    Client::handleRedirect(Request req) {
	std::string path = req.getPath().substr(1);
	std::map<std::string, locationLevel>::iterator it = req.getConf().locations.begin();
	for ( ; it != req.getConf().locations.end() ; it++) {
		if (it->first == path) {
			sendRedirect(*this, it->second.redirectionHTTP.first, it->second.redirectionHTTP.second);
			return 0;
		}
	}
	return 1;
}

int Client::viewDirectory(std::string fullPath, Request& req) {
	locationLevel* loc = NULL;
	if (matchLocation(req.getPath(), req.getConf(), loc)) {
		if (loc->autoindex == true) {
			return createDirList(fullPath, req);
		} else {
			std::string indexPath = matchAndAppendPath(fullPath, loc->indexFile);
			struct stat indexStat;
			if (stat(indexPath.c_str(), &indexStat) == 0 && S_ISREG(indexStat.st_mode)) {
				Request req;
				req.setPath(indexPath);
				if (buildBody(*this, req, indexPath) == 1)
					return 1;
				req.setContentType("text/html");
				_connect = "keep-alive";
				sendResponse(*this, req, "keep-alive", req.getBody());
				std::cout << getTimeStamp(_fd) << GREEN  << "Successfully served index file: " << RESET << indexPath << std::endl;
				return 0;
			} else {
				std::cerr << getTimeStamp(_fd) << RED  << "Autoindex off and no index.html: " << RESET << fullPath << std::endl;
				sendErrorResponse(*this, 403, req);
				return 1;
			}
		}
	} 
	else if (req.getConf().locations.find("/") != req.getConf().locations.end()) {
		if (req.getConf().locations.find("/")->second.autoindex) {
			return createDirList(fullPath, req);
		} else {
			std::string indexPath = matchAndAppendPath(fullPath, loc->indexFile);
			struct stat indexStat;
			if (stat(indexPath.c_str(), &indexStat) == 0 && S_ISREG(indexStat.st_mode)) {
				Request req;
				req.setPath(indexPath);
				if (buildBody(*this, req, indexPath) == 1)
					return 1;
				req.setContentType("text/html");
				_connect = "keep-alive";
				sendResponse(*this, req, "keep-alive", req.getBody());
				std::cout << getTimeStamp(_fd) << GREEN  << "Successfully served index file: " << RESET << indexPath << std::endl;
				return 0;
			} else {
				std::cerr << getTimeStamp(_fd) << RED  << "Autoindex off and no index.html: " << RESET << fullPath << std::endl;
				sendErrorResponse(*this, 403, req);
				return 1;
			}
		}
	}
	else {
		std::cerr << getTimeStamp(_fd) << RED << "No matching location for: " << RESET << req.getPath() << std::endl;
		sendErrorResponse(*this, 403, req);
		return 1;
	}
	return 0;
}

int Client::createDirList(std::string fullPath, Request& req) {
	std::string dirListing = showDir(fullPath, req.getPath());
	if (dirListing.empty()) {
		sendErrorResponse(*this, 403, req);
		return 1;
	}
	std::string response = "HTTP/1.1 200 OK\r\n";
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: " + tostring(dirListing.length()) + "\r\n";
	response += "Connection: keep-alive\r\n";
	response += "\r\n";
	response += dirListing;
	_connect = "keep-alive";
	addSendBuf(*_webserv, _fd, response);
	setEpollEvents(*_webserv, _fd, EPOLLOUT);
	std::cout << getTimeStamp(_fd) << GREEN  << "Sent directory listing: " << RESET << fullPath << std::endl;
	return 0;
}

std::string Client::showDir(const std::string& dirPath, const std::string& requestUri) {
	DIR* dir = opendir(dirPath.c_str());
	if (!dir)
		return "";
	
	std::string html = "<!DOCTYPE html>\n"
		"<html>\n"
		"<head>\n"
		"    <title>Index of " + requestUri + "</title>\n"
		"    <style>\n"
		"        body { font-family: Arial, sans-serif; margin: 20px; line-height: 1.6; }\n"
		"        h1 { color: #333; border-bottom: 1px solid #eee; padding-bottom: 10px; }\n"
		"        ul { list-style-type: none; padding: 0; }\n"
		"        li { padding: 8px; border-bottom: 1px solid #f2f2f2; }\n"
		"        li:hover { background-color: #f8f8f8; }\n"
		"        a { text-decoration: none; color: #0366d6; }\n"
		"        a:hover { text-decoration: underline; }\n"
		"        .header { background-color: #f4f4f4; padding: 10px; margin-bottom: 20px; border-radius: 4px; }\n"
		"        .server-info { font-size: 12px; color: #777; margin-top: 20px; }\n"
		"    </style>\n"
		"</head>\n"
		"<body>\n"
		"    <div class=\"header\">\n"
		"        <h1>Index of " + requestUri + "</h1>\n"
		"    </div>\n"
		"    <ul>\n";
	
	if (requestUri != "/") {
		std::string parentUri = encode(requestUri);
		parentUri = "/" + matchAndAppendPath(parentUri, "/../") + "/";
		html += "        <li><a href=\"" + parentUri + "\">Parent Directory</a></li>\n";
	}
	
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;
		
		if (name == "." || name == "..")
			continue;
		
		std::string entryPath = dirPath + "/" + name;
		struct stat entryStat;
		if (stat(entryPath.c_str(), &entryStat) == 0) {
			if (S_ISDIR(entryStat.st_mode))
				name += "/";
		}
		
		html += "        <li><a href=\"" + encode(requestUri);
		if (requestUri[requestUri.length() - 1] != '/')
			html += "/";
		html += encode(name) + "\">" + name + "</a></li>\n";
	}
	
	html += "    </ul>\n"
		"    <div class=\"server-info\">\n"
		"        <p>WebServ 1.0</p>\n"
		"    </div>\n"
		"</body>\n"
		"</html>";
		
	closedir(dir);
	return html;
}

bool Client::saveFile(Request& req, const std::string& filename, const std::string& content) {
	std::string fullPath = matchAndAppendPath(_server->getUploadDir(*this, req), filename);
	if (fullPath.empty())
		return false;
	if (!ensureUploadDirectory(*this, req)) {
		std::cerr << getTimeStamp(_fd) << RED << "Error: Failed to ensure upload directory exists" << RESET << std::endl;
		return false;
	}
	
	int fd = open(fullPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		std::cerr << getTimeStamp(_fd) << RED << "Error: Failed to open file for writing: " << RESET << fullPath << std::endl;
		return false;
	}
	
	ssize_t bytesWritten = write(fd, content.c_str(), content.length());
	if (!checkReturn(_fd, bytesWritten, "write()", "Failed to write to file")) {
		sendErrorResponse(*this, 500, req);
		close(fd);
		return false;
	}
	close(fd);
	return true;
}
