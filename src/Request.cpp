#include "../include/Request.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"

Request::Request() {}

Request::Request(const std::string& rawRequest, Client& client, int clientFd) : 
	_host(""),
	_method("GET"),
	_check(""),
	_path(""),
	_contentType(""),
	_version(""),
	_headers(),
	_body(""),
	_query(""),
	_boundary(""),
	_contentLength(0),
	_curConf(),
	_client(&client),
	_configs(_client->getConfigs()),
	_clientFd(clientFd)
{
	parse(rawRequest);
}

Request::Request(const Request& copy) {
	*this = copy;
}

Request &Request::operator=(const Request& copy) {
	if (this != &copy) {
		_host = copy._host;
		_method = copy._method;
		_check = copy._check;
		_path = copy._path;
		_contentType = copy._contentType;
		_version = copy._version;
		_headers = copy._headers;
		_body = copy._body;
		_query = copy._query;
		_boundary = copy._boundary;
		_contentLength = copy._contentLength;
		_curConf = copy._curConf;
		_client = copy._client;
		_configs = copy._configs;
		_clientFd = copy._clientFd;
	}
	return *this;
}

Request::~Request() {}

std::string &Request::getPath() {
	return _path;
}

std::string const &Request::getMethod() {
	return _method;
}

std::string const &Request::getCheck() {
	return _check;
}

std::string const &Request::getVersion() {
	return _version;
}

std::string const &Request::getBody() {
	return _body;
}

std::string const &Request::getContentType() {
	return _contentType;
}

std::string const &Request::getQuery() {
	return _query;
}

std::string const &Request::getBoundary() {
	return _boundary;
}

std::map<std::string, std::string> &Request::getHeaders() {
	return _headers;
}

size_t	&Request::getContentLength() {
	return _contentLength;
}

serverLevel& Request::getConf() {
	return _curConf;
}

void    Request::setPath(std::string const path) {
	_path = path;
}

void    Request::setBody(std::string const body) {
	_body = body;
}

void    Request::setContentType(std::string const content) {
	_contentType = content;
}

bool Request::hasServerName() {
	std::map<std::string, std::string>::iterator it = _headers.find("Host");
	std::string servName;
	if (it != _headers.end())
		servName = it->second;
	if (servName.find("localhost") != std::string::npos) {
		std::string portPart = servName.substr(servName.find(":") + 1);
		if (onlyDigits(portPart)) {
			std::string sum = "localhost:" + portPart;
			if (sum == servName)
				return false;
		}
	}
	return true;
}

bool Request::matchHostServerName() {
	std::map<std::string, std::string>::iterator it = _headers.find("Host");
	std::string servName;
	if (it != _headers.end())
		servName = it->second;
	if (hasServerName()) {
		for (size_t i = 0; i < _configs.size(); i++) {
			for (size_t j = 0; j < _configs[i].servName.size(); j++) {
				if (servName == _configs[i].servName[j]) {
					_curConf = _configs[i];
					return true;
				}
			}
		}
		for (size_t i = 0; i < _configs.size(); i++) {
			for (size_t j = 0; j < _configs[i].port.size(); j++) {
				if (_configs[i].port[j].second == true) {
					_curConf = _configs[i];
					return true;
				}
			}
		}
	}
	else {
		int portPart = std::atoi(servName.substr(servName.find(":") + 1).c_str());
		for (size_t i = 0; i < _configs.size(); i++) {
			for (size_t j = 0; j < _configs[i].port.size(); j++) {
				if (_configs[i].port[j].first.second == portPart) {
					_curConf = _configs[i];
					return true;
				}
			}
		}
	}
	return false;
}

void Request::parse(const std::string& rawRequest) {
	if (_client->cgiIsDone() == false && rawRequest.empty())
		return;
	if (rawRequest.empty()) {
		std::cerr << getTimeStamp(_clientFd) << RED << "Empty request!" << RESET << std::endl;
		_check = "BAD";
		return;
	}

	checkContentLength(rawRequest);

	size_t headerEnd = rawRequest.find("\r\n\r\n");
	size_t headerSeparatorLength = 4;
	
	if (headerEnd == std::string::npos) {
		headerEnd = rawRequest.find("\n\n");
		headerSeparatorLength = 2;
		if (headerEnd == std::string::npos) {
			headerEnd = rawRequest.length();
			headerSeparatorLength = 0;
		}
	}

	std::string headerSection = rawRequest.substr(0, headerEnd);
	if (headerEnd + headerSeparatorLength < rawRequest.length()) {
		_body = rawRequest.substr(headerEnd + headerSeparatorLength);
	}
	
	if (!parseHeaders(headerSection)) {
		std::cerr << getTimeStamp(_clientFd) << RED << "Invalid HTTP Request: Missing Host Header" << RESET << std::endl;
		_check = "BAD";
		return;
	}
	if (!matchHostServerName()) {
		std::cerr << getTimeStamp(_clientFd) << RED << "No Host-ServerName match + no default config specified!" << RESET << std::endl;
		_check = "BAD";
		return;
	}
	parseContentType();

	std::istringstream iss(headerSection);
	std::string requestLine;
	std::getline(iss, requestLine);
	size_t end = requestLine.find_last_not_of(" \t\r\n");
	if (end != std::string::npos)
		requestLine = requestLine.substr(0, end + 1);

	std::istringstream lineStream(requestLine);
	std::string target;
	lineStream >> _method >> target >> _version;

	if (_method.empty() || (_method != "GET" && target.empty()) || _version.empty() || _version != "HTTP/1.1") {
		_check = "BAD";
		return;
	}

	size_t queryPos = target.find('?');
	if (queryPos != std::string::npos) {
		_path = target.substr(0, queryPos);
		_query = target.substr(queryPos + 1);
	} else {
		_path = target;
		_query = "";
	}
	if ((_path == "/" || _path == "") && _method == "GET") {
		std::map<std::string, locationLevel>::iterator it = _curConf.locations.find("/");
		if (it != _curConf.locations.end())
			_path = matchAndAppendPath(it->second.rootLoc, it->second.indexFile);
	}

	end = _path.find_last_not_of(" \t\r\n");
	if (end != std::string::npos)
		_path = _path.substr(0, end + 1);

	if (_method == "GET" || _method == "POST" || _method == "DELETE"
		|| _method == "PUT" || _method == "HEAD" || _method == "OPTIONS"
		|| _method == "PATCH" || _method == "TRACE" || _method == "CONNECT") {
		if (_method != "GET" && _method != "POST" && _method != "DELETE") {
			_check = "NOTALLOWED";
			return;
		}
	} else {
		_check = "BAD";;
		return;
	}

	if (isChunkedTransfer()) {
		if (_method == "POST") {
			std::cout << getTimeStamp(_clientFd) << BLUE << "POST request with chunked body: " << RESET << _body.length() 
					<< " bytes of chunked data" << std::endl;
		}
	}
	_path = decode(_path);
}

bool Request::parseHeaders(const std::string& headerSection) {
	std::istringstream iss(headerSection);
	std::string line;
	bool host = false;
	
	std::getline(iss, line);
	
	while (std::getline(iss, line) && !line.empty() && line != "\r") {
		size_t colonPos = line.find(':');
		if (colonPos != std::string::npos) {
			std::string key = line.substr(0, colonPos);
			std::string value = line.substr(colonPos + 1);
			
			key.erase(0, key.find_first_not_of(" \t"));
			key.erase(key.find_last_not_of(" \t\r\n") + 1);
			value.erase(0, value.find_first_not_of(" \t"));
			value.erase(value.find_last_not_of(" \t\r\n") + 1);
			if (key == "Host")
				host = true;
			_headers[key] = value;
		}
	}
	return host;
}

void Request::checkContentLength(std::string buffer) {
	size_t pos = buffer.find("Content-Length:");
	if (pos != std::string::npos) {
		pos += 15;
		while (pos < buffer.size() && (buffer[pos] == ' ' || buffer[pos] == '\t'))
			pos++;
		
		size_t eol = buffer.find("\r\n", pos);
		if (eol == std::string::npos)
			eol = buffer.find("\n", pos);
		
		if (eol != std::string::npos) {
			std::string valueStr = buffer.substr(pos, eol - pos);
			_contentLength = strtoul(valueStr.c_str(), NULL, 10);
			return;
		}
	}
	
	pos = buffer.find("Transfer-Encoding:");
	if (pos != std::string::npos) {
		size_t eol = buffer.find("\r\n", pos);
		if (eol == std::string::npos)
			eol = buffer.find("\n", pos);
		
		if (eol != std::string::npos) {
			std::string transferEncoding = buffer.substr(pos + 18, eol - pos - 18);
			if (transferEncoding.find("chunked") != std::string::npos) {
				_contentLength = 0;
				std::cout << getTimeStamp(_clientFd) << BLUE << "Transfer-Encoding: chunked detected" << RESET << std::endl;
			}
		}
	}
}

void Request::parseContentType() {
	std::map<std::string, std::string>::iterator it = _headers.find("Content-Type");
	if (it != _headers.end()) {
		_contentType = it->second;
		
		if (_contentType.find("multipart/form-data") != std::string::npos) {
			size_t boundaryPos = _contentType.find("boundary=");
			if (boundaryPos != std::string::npos) {
				std::string boundary = _contentType.substr(boundaryPos + 9);
				
				if (!boundary.empty() && boundary[0] == '"') {
					size_t endQuote = boundary.find('"', 1);
					if (endQuote != std::string::npos) {
						boundary = boundary.substr(1, endQuote - 1);
					}
				}
				_boundary = boundary;
			}
		}
	} else if (_method == "POST")
		_contentType = "application/octet-stream";
}

std::string Request::getMimeType(std::string const &path) {
	std::string ext;
	size_t dotPos = path.find_last_of(".");
	
	if (dotPos != std::string::npos) {
		ext = path.substr(dotPos + 1);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	} else if (path == "/" || path.empty())
		return "text/html";
	else
		return "text/plain";

	if (ext == "html" || ext == "htm")
		return "text/html";
	else if (ext == "css")
		return "text/css";
	else if (ext == "js") 
		return "application/javascript";
	else if (ext == "jpg" || ext == "jpeg") 
		return "image/jpeg";
	else if (ext == "png")
		return "image/png";
	else if (ext == "gif")
		return "image/gif";
	else if (ext == "svg")
		return "image/svg+xml";
	else if (ext == "ico")
		return "image/vnd.microsoft.icon";
	else if (ext == "pdf")
		return "application/pdf";
	else if (ext == "json")
		return "application/json";
	else if (ext == "xml")
		return "application/xml";
	else if (ext == "txt")
		return "text/plain";
	else if (ext == "mp4")
		return "video/mp4";
	else if (ext == "mp3")
		return "audio/mpeg";
	
	return "application/octet-stream";
}

bool Request::isChunkedTransfer() const {
	std::map<std::string, std::string>::const_iterator it = _headers.find("Transfer-Encoding");
	if (it != _headers.end() && it->second.find("chunked") != std::string::npos)
		return true;
	return false;
}