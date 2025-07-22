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
	_server = &serv;
	_webserv = &serv.getWebServ();
	_req = NULL;
	_configs = serv.getConfigs();
	_sendOffset = 0;
	_exitErr = false;
	_fileIsNew = false;
	_shouldClose = false;
	_lastUsed = time(NULL);//TODO: lra
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
		_server = other._server;
		_webserv = other._webserv;
		_req = other._req;
		_configs = other._configs;
		_sendOffset = other._sendOffset;
		_exitErr = other._exitErr;
		_fileIsNew = other._fileIsNew;
		_shouldClose = other._shouldClose;
		_lastUsed = other._lastUsed; //TODO: lra
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

Request& Client::getRequest() {
	return *_req;
}

size_t& Client::getOffset() {
	return _sendOffset;
}

bool &Client::exitErr() {
	return _exitErr;
}

std::vector<serverLevel> Client::getConfigs() {
	return _configs;
}

bool &Client::fileIsNew() {
	return _fileIsNew;
}

bool &Client::shouldClose() {
	return _shouldClose;
}

time_t &Client::lastUsed() {//TODO: lra
	return _lastUsed;
}

int Client::acceptConnection(int serverFd) {
	std::cout << GREY << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << RESET << std::endl;
	_addrLen = sizeof(_addr);
	_fd  = accept(serverFd, (struct sockaddr *)&_addr, &_addrLen);
	if (_fd < 0) {
		std::cerr << getTimeStamp() << RED << "Error: accept() failed" << RESET << std::endl;
		return 1;
	}
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
		_exitErr = true;
		std::cerr << getTimeStamp(_fd) << RED << "Error: recv() failed" << RESET << std::endl;
		return;
	}
	
	if (bytesRead == 0) {
		_exitErr = true;
		return;
	}
	
	_requestBuffer.append(buffer, bytesRead);

	if (_requestBuffer.find("\r\n\r\n") == std::string::npos && 
		_requestBuffer.find("\n\n") == std::string::npos) {
		std::cout << getTimeStamp(_fd) << BLUE 
				<< "Headers incomplete, waiting for more data" << RESET << std::endl;
		_exitErr = false;
		return;
	}

	bool isComplete = false;
	bool isChunked = (iFind(_requestBuffer, "Transfer-Encoding:") != std::string::npos &&
					iFind(_requestBuffer, "chunked") != std::string::npos);
	bool hasContentLength = (iFind(_requestBuffer, "Content-Length:") != std::string::npos);

	if (isChunked)
		isComplete = isChunkedBodyComplete(_requestBuffer);
	else if (hasContentLength)
		isComplete = (checkLength(_requestBuffer, _fd, printNewLine) == 1);
	else
		isComplete = true;
	
	if (!isComplete) {
		_exitErr = false;
		return;
	}

	if (printNewLine == true)
		std::cout << std::endl;
	std::cout << getTimeStamp(_fd) << GREEN  << "Complete request received, processing..." << RESET << std::endl;
	printNewLine = false;
	
	Request req(_requestBuffer, *this, _fd);
	_req = &req;
	_exitErr = processRequest();
	if (_exitErr != 1)
		_requestBuffer.clear();
	_shouldClose = false;
	if (shouldCloseConnection(*_req))
		_shouldClose = true;		
}

int Client::processRequest() {
	serverLevel &conf = _req->getConf();

	if (_req->getContentLength() > conf.requestLimit) {
		_req->statusCode() = 413;
		sendErrorResponse(*this, *_req);
		_requestBuffer.clear();
		return 1;
	}

	if (_req->getCheck() == "BAD") {
		sendErrorResponse(*this, *_req);
		_requestBuffer.clear();
		return 1;
	}
	
	if (_req->getCheck() == "NOTALLOWED") {
		std::cerr << getTimeStamp(_fd) << RED  << "Method Not Allowed: " << RESET << _req->getMethod() << std::endl;
		sendErrorResponse(*this, *_req);
		_requestBuffer.clear();
		return 1;
	}

	locationLevel* loc = NULL;
	if (matchLocation(_req->getPath(), conf, loc)) {
		if (loc->hasRedirect == true) {
			_req->statusCode() = loc->redirectionHTTP.first;
			sendRedirect(*this, loc->redirectionHTTP.second, *_req);
			return 0;
		}
	}
	if (iFind(_req->getContentType(), "multipart/form-data") != std::string::npos) {
		int result = handleMultipartPost();
		if (result != -1)
			return result;
		return 0;
	}
	std::cout << getTimeStamp(_fd) << BLUE << "Parsed Request: " << RESET << 
		_req->getMethod() << " " << _req->getPath() << " " << _req->getVersion() << std::endl;

	int ret = 1;
	if (_req->getMethod() == "GET")
		ret = handleGetRequest();
	else if (_req->getMethod() == "POST")
		ret = handlePostRequest();
	else if (_req->getMethod() == "DELETE")
		ret = handleDeleteRequest();
	else {
		sendErrorResponse(*this, *_req);
		ret = 1;
	}
	return ret;
}

int Client::handleGetRequest() {
	locationLevel* loc = NULL;
	if (!matchLocation(_req->getPath(), _req->getConf(), loc)) {
		std::cerr << getTimeStamp(_fd) << RED  << "Location not found: " << RESET << _req->getPath() << std::endl;
		_req->statusCode() = 404;
		sendErrorResponse(*this, *_req);
		return 1;
	}
	if (_req->getPath().find("../") != std::string::npos) {
		_req->statusCode() = 403;
		sendErrorResponse(*this, *_req);
		return 1;
	}
	if (loc->autoindex == true && isFileBrowserRequest(_req->getPath()))
		return handleFileBrowserRequest();
	else
		return handleRegularRequest();
}

int Client::handlePostRequest() {
	locationLevel* loc = NULL;
	if (!matchLocation(_req->getPath(), _req->getConf(), loc)) {
		std::cerr << getTimeStamp(_fd) << RED  << "Location not found for POST request: " 
				<< RESET << _req->getPath() << std::endl;
		_req->statusCode() = 404;
		sendErrorResponse(*this, *_req);
		return 1;
	}
	std::string fullPath = getLocationPath(*this, *_req, "POST");
	if (fullPath.empty()) {
		_req->statusCode() = 404;
		sendErrorResponse(*this, *_req);
		return 1;
	}
	if (isCGIScript(_req->getPath())) {
		CGIHandler* cgi = new CGIHandler(this);
		std::string cgiPath = matchAndAppendPath(_server->getWebRoot(*_req, *loc), _req->getPath());
		cgi->setPath(cgiPath);
		int result = cgi->executeCGI(*_req);
		if (result != 0)
			delete cgi;
		return result;
	}

	if (iFind(_req->getContentType(), "multipart/form-data") != std::string::npos)
		return handleMultipartPost();
	
	if (_req->getPath().find("../") != std::string::npos) {
		_req->statusCode() = 403;
		sendErrorResponse(*this, *_req);
		return 1;
	}
	
	std::string contentToWrite;
	
	if (isChunkedRequest(*_req)) {
		std::cout << getTimeStamp(_fd) << BLUE << "Processing chunked request" << RESET << std::endl;
		contentToWrite = decodeChunkedBody(_fd, _req->getBody());
		
		if (contentToWrite.empty()) {
			_req->statusCode() = 400;
			std::cerr << getTimeStamp(_fd) << RED  << "Failed to decode chunked data" << RESET << std::endl;
			sendErrorResponse(*this, *_req);
			return 1;
		}
	} else {
		contentToWrite = _req->getBody();
		std::string fileName;
		if (contentToWrite.empty() && !_req->getQuery().empty())
			doQueryStuff(_req->getQuery(), fileName, contentToWrite);
		else
			doQueryStuff(_req->getBody(), fileName, contentToWrite);
		fullPath = matchAndAppendPath(fullPath, fileName);
	}
	if (!tryLockFile(fullPath, _fd, _fileIsNew)) {
		_req->statusCode() = 423;
		sendErrorResponse(*this, *_req);
		return 1;
	}
	int fd = open(fullPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		releaseLockFile(fullPath);
		_req->statusCode() = 418;
		std::cerr << getTimeStamp(_fd) << RED << "Failed to open file for writing: " << RESET << fullPath << std::endl;
		sendErrorResponse(*this, *_req);
		if (_fileIsNew == true) {
			remove(fullPath.c_str());
			std::cerr << getTimeStamp(_fd) << RED << "File removed: " + fullPath << RESET << std::endl;
		}
		return 1;
	}
	
	std::cout << getTimeStamp(_fd) << BLUE << "Writing to file: " << RESET << fullPath << std::endl;
	
	ssize_t bytesWritten = write(fd, contentToWrite.c_str(), contentToWrite.length());
	releaseLockFile(fullPath);
	if (bytesWritten < 0) {
		_req->statusCode() = 500;
		std::cerr << getTimeStamp(_fd) << RED << "Error: write() failed" << RESET << std::endl;
		sendErrorResponse(*this, *_req);
		if (_fileIsNew == true) {
			remove(fullPath.c_str());
			std::cerr << getTimeStamp(_fd) << RED << "File removed: " + fullPath << RESET << std::endl;
		}
		close(fd);
		return 1;
	}
	std::string responseBody = "File uploaded successfully. Wrote " + tostring(bytesWritten) + " bytes.";
	_req->statusCode() = 201;
	sendResponse(*this, *_req, responseBody);
	std::cout << getTimeStamp(_fd) << GREEN  << "Uploaded file: " << RESET << fullPath 
			<< " (" << bytesWritten << " bytes written)" << std::endl;
	close(fd);
	return 0;
}

int Client::handleDeleteRequest() {
	std::string fullPath = getLocationPath(*this, *_req, "DELETE");
	if (fullPath.empty()) {
		_req->statusCode() = 404;
		sendErrorResponse(*this, *_req);
		return 1;
	}
    if (isCGIScript(_req->getPath())) {
    	CGIHandler* cgi = new CGIHandler(this);
		cgi->setPath(fullPath);        
		int result = cgi->executeCGI(*_req);
		if (result != 0)
			delete cgi;
		return result;
	}

	size_t end = fullPath.find_last_not_of(" \t\r\n");
	if (end != std::string::npos)
		fullPath = fullPath.substr(0, end + 1);

	if (remove(fullPath.c_str()) != 0) {
		if (errno == ENOENT) {
			_req->statusCode() = 404;
			sendErrorResponse(*this, *_req);
			return 1;
		} else if (errno == EACCES || errno == EPERM) {
			_req->statusCode() = 403;
			sendErrorResponse(*this, *_req);
			return 1;
		} else {
			_req->statusCode() = 500;
			std::cerr << getTimeStamp(_fd) << RED  << "Error deleting file: " << RESET << fullPath
					<< " - " << strerror(errno) << std::endl;
			sendErrorResponse(*this, *_req);
			return 1;
		}
	}
	sendResponse(*this, *_req, "");
	return 0;
}

int Client::handleFileBrowserRequest() {
	std::string requestPath = _req->getPath();
	std::string actualPath;
	if (requestPath == "/root" || requestPath == "/root/")
		actualPath = "/";
	else if (requestPath.find("/root/") == 0) {
		actualPath = requestPath.substr(5);
		if (actualPath.empty()) actualPath = "/";
	} else {
		locationLevel* loc = NULL;
		matchLocation(requestPath, _req->getConf(), loc);
		actualPath = requestPath.substr(5);
		std::string actualFullPath = matchAndAppendPath(_server->getWebRoot(*_req, *loc), actualPath);
		
		struct stat fileStat;
		if (stat(actualFullPath.c_str(), &fileStat) != 0) {
			_req->statusCode() = 404;
			std::cerr << getTimeStamp(_fd) << RED  << "File not found: " << RESET << actualFullPath  << std::endl;
			sendErrorResponse(*this, *_req);
			return 1;
		}
		if (S_ISDIR(fileStat.st_mode)) {
			_req->setPath(actualPath);
			return createDirList(actualFullPath, *_req);
		} else if (S_ISREG(fileStat.st_mode)) {
			if (buildBody(*this, *_req, actualFullPath) == 1)
				return 1;
			_req->setContentType(_req->getMimeType(actualFullPath));
			sendResponse(*this, *_req, _req->getBody());
			std::cout << getTimeStamp(_fd) << GREEN  << "Successfully served file from browser: " 
					<< RESET << actualFullPath << std::endl;
			return 0;
		} else {
			_req->statusCode() = 403;
			std::cerr << getTimeStamp(_fd) << RED << "Not a regular file or directory: " << RESET << actualFullPath << std::endl;
			sendErrorResponse(*this, *_req);
			return 1;
		}
	}
	return 0;
}

int Client::handleRegularRequest() {
	locationLevel* loc = NULL;
	if (!matchLocation(_req->getPath(), _req->getConf(), loc)) {
		_req->statusCode() = 404;
		std::cerr << getTimeStamp(_fd) << RED  << "Location not found: " << RESET << _req->getPath() << std::endl;
		sendErrorResponse(*this, *_req);
		return 1;
	}
	std::string reqPath = _req->getPath();
	if (reqPath == "/" || reqPath.empty())
		reqPath = loc->indexFile;

	size_t end = reqPath.find_last_not_of(" \t\r\n");
	if (end != std::string::npos)
		reqPath = reqPath.substr(0, end + 1);
	std::string fullPath;
	if (reqPath.find("/home") == std::string::npos)
		fullPath = matchAndAppendPath(_server->getWebRoot(*_req, *loc), reqPath);
	else
		fullPath = reqPath;

	if (fullPath.find("root") != std::string::npos && loc->autoindex == false) {
		_req->statusCode() = 403;
		std::cerr << getTimeStamp(_fd) << RED << "Access to directory browser is forbidden when autoindex is off" << RESET << std::endl;
		sendErrorResponse(*this, *_req);
		return 1;
	}

	if (handleRedirect() == 0)
		return 1;
		
	if (isCGIScript(reqPath)) {      
		CGIHandler* cgi = new CGIHandler(this);
		cgi->setCGIBin(&_req->getConf());
		std::string fullCgiPath = matchAndAppendPath(_server->getWebRoot(*_req, *loc), reqPath);
		cgi->setPath(fullCgiPath);
		int result = cgi->executeCGI(*_req);
		if (result != 0)
			delete cgi;
		return result;
	}

	std::cout << getTimeStamp(_fd) << BLUE << "Handling GET request for path: " << RESET << _req->getPath() << std::endl;

	struct stat fileStat;
	if (stat(fullPath.c_str(), &fileStat) != 0) {
		translateErrorCode(errno, _req->statusCode());
		sendErrorResponse(*this, *_req);
		return 1;
	}
	
	if (S_ISDIR(fileStat.st_mode))
		return viewDirectory(fullPath, *_req);
	else if (!S_ISREG(fileStat.st_mode)) {
		_req->statusCode() = 403;
		std::cerr << getTimeStamp(_fd) << RED << "Not a regular file: " << RESET << fullPath << std::endl;
		sendErrorResponse(*this, *_req);
		return 1;
	}
	
	if (buildBody(*this, *_req, fullPath) == 1)
		return 1;

	std::string contentType = _req->getMimeType(fullPath);
	if (fullPath.find(".html") != std::string::npos || reqPath == "/" || reqPath == loc->indexFile)
		contentType = "text/html";

	_req->setContentType(contentType);
	sendResponse(*this, *_req, _req->getBody());
	std::cout << getTimeStamp(_fd) << GREEN  << "Sent file: " << RESET << fullPath << std::endl;
	return 0;
}

int Client::handleMultipartPost() {
	std::string boundary = _req->getBoundary();
	
	if (boundary.empty()) {
		_req->statusCode() = 400;
		sendErrorResponse(*this, *_req);
		return 1;
	}
	
	Multipart parser(_requestBuffer, boundary);
	
	if (!parser.parse()) {
		_req->statusCode() = 400;
		sendErrorResponse(*this, *_req);
		return 1;
	}
	
	std::string filename = parser.getFilename();
	if (filename.empty()) {
		_req->statusCode() = 400;
		sendErrorResponse(*this, *_req);
		return 1;
	}

	std::string fileContent = parser.getFileContent();
	if (fileContent.empty() && !parser.isComplete()) {
		_req->statusCode() = 400;
		sendErrorResponse(*this, *_req);
		return 1;
	}
	
	if (!saveFile(*_req, filename, fileContent)) {
		_req->statusCode() = 500;
		sendErrorResponse(*this, *_req);
		return 1;
	}
	std::cout << getTimeStamp(_fd) << GREEN  << "Received: " + parser.getFilename() << RESET << std::endl;
	std::string successMsg = "Successfully uploaded file:" + filename;
	_req->statusCode() = 201;
	sendResponse(*this, *_req, successMsg);
	std::cout << getTimeStamp(_fd) << GREEN  << "File transfer ended" << RESET << std::endl;    
	return 0;
}

int    Client::handleRedirect() {
	std::string path = _req->getPath().substr(1);
	std::map<std::string, locationLevel>::iterator it = _req->getConf().locations.begin();
	for ( ; it != _req->getConf().locations.end() ; it++) {
		if (it->first == path) {
			_req->statusCode() = it->second.redirectionHTTP.first;
			sendRedirect(*this, it->second.redirectionHTTP.second, *_req);
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
				sendResponse(*this, req, req.getBody());
				std::cout << getTimeStamp(_fd) << GREEN  << "Successfully served index file: " << RESET << indexPath << std::endl;
				return 0;
			} else {
				req.statusCode() = 403;
				std::cerr << getTimeStamp(_fd) << RED  << "Autoindex off and no index.html: " << RESET << fullPath << std::endl;
				sendErrorResponse(*this, req);
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
				sendResponse(*this, req, req.getBody());
				std::cout << getTimeStamp(_fd) << GREEN  << "Successfully served index file: " << RESET << indexPath << std::endl;
				return 0;
			} else {
				req.statusCode() = 403;
				std::cerr << getTimeStamp(_fd) << RED  << "Autoindex off and no index.html: " << RESET << fullPath << std::endl;
				sendErrorResponse(*this, req);
				return 1;
			}
		}
	}
	else {
		req.statusCode() = 403;
		std::cerr << getTimeStamp(_fd) << RED << "No matching location for: " << RESET << req.getPath() << std::endl;
		sendErrorResponse(*this, req);
		return 1;
	}
	return 0;
}

int Client::createDirList(std::string fullPath, Request& req) {
	std::string dirListing = showDir(fullPath, req.getPath());
	if (dirListing.empty()) {
		req.statusCode() = 404;
		sendErrorResponse(*this, req);
		return 1;
	}
	std::string response = "HTTP/1.1 200 OK\r\n";
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: " + tostring(dirListing.length()) + "\r\n";
	if (shouldCloseConnection(req))
		response += "Connection: close\r\n";
	response += "\r\n";
	response += dirListing;
	response += "\n"; //added for better netcat output
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
	if (fullPath.empty()) {
		req.statusCode() = 404;
		return false;
	}
	if (!ensureUploadDirectory(*this, req)) {
		req.statusCode() = 500;
		std::cerr << getTimeStamp(_fd) << RED << "Error: Failed to ensure upload directory exists" << RESET << std::endl;
		return false;
	}
	
	if (!tryLockFile(fullPath, _fd, _fileIsNew)) {
		req.statusCode() = 423;
		sendErrorResponse(*this, req);
		return false;
	}

	int fd = open(fullPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		req.statusCode() = 500;
		std::cerr << getTimeStamp(_fd) << RED << "Error: Failed to open file for writing: " << RESET << fullPath << std::endl;
		return false;
	}
	
	ssize_t bytesWritten = write(fd, content.c_str(), content.length());
	if (!checkReturn(_fd, bytesWritten, "write()")) {
		releaseLockFile(fullPath);
		req.statusCode() = 500;
		sendErrorResponse(*this, req);
		close(fd);
		return false;
	}
	close(fd);
	releaseLockFile(fullPath);
	return true;
}
