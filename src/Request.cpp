#include "../include/Request.hpp"
#include "../include/ConfigHelper.hpp"
#include "../include/Server.hpp"
#include "../include/Helper.hpp"
#include "../include/Client.hpp"

Request::Request() {}

Request::Request(const std::string& rawRequest, Client& client, int clientFd) : 
	_host(""),
	_method(""),
	_check(""),
	_path(""),
	_contentType(""),
	_version(""),
	_body(""),
	_query(""),
	_boundary(""),
	_contentLength(0),
	_headers(),
	_curConf(),
	_client(&client),
	_configs(_client->getConfigs()),
	_clientFd(clientFd),
	_hasLengthOrIsChunked(false)
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
		_hasLengthOrIsChunked = copy._hasLengthOrIsChunked;
	}
	return *this;
}

Request::~Request() {}

std::string &Request::getHost() {
	return _host;
}

std::string &Request::getPath() {
	return _path;
}

std::string const &Request::getMethod() {
	return _method;
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

size_t	&Request::getContentLength() {
	return _contentLength;
}

serverLevel& Request::getConf() {
	return _curConf;
}

std::map<std::string, std::string> &Request::getHeaders() {
	return _headers;
}

bool &Request::hasLengthOrIsChunked() {
	return _hasLengthOrIsChunked;
}

std::string &Request::check() {
	return _check;
}

Client &Request::getClient() {
	return *_client;
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
	if (iFind(_host, "localhost") != std::string::npos) {
		std::string portPart = _host.substr(_host.find(":") + 1);
		if (onlyDigits(portPart)) {
			std::string sum = "localhost:" + portPart;
			if (iEqual(sum, _host))
				return false;
		}
	}
	return true;
}

bool Request::matchHostServerName() {
	if (hasServerName()) {
		for (size_t i = 0; i < _configs.size(); i++) {
			for (size_t j = 0; j < _configs[i].servName.size(); j++) {
				if (iEqual(_host, _configs[i].servName[j])) {
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
		int portPart = std::atoi(_host.substr(_host.find(":") + 1).c_str());
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
	if (rawRequest.empty()) {
		_client->output() = getTimeStamp(_clientFd) + RED + "Empty request!" + RESET;
		_client->statusCode() = 400;
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
	
	parseHeaders(headerSection);
	if (_client->statusCode() >= 400) {
		_check = "BAD";
		return;
	}

	if (!matchHostServerName()) {
		_client->output() = getTimeStamp(_clientFd) + RED + "No Host-ServerName match + no default config specified!" + RESET;
		_client->statusCode() = 404;
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

	if (_method.empty() || (_method != "GET" && target.empty())) {
		_client->statusCode() = 400;
		_check = "BAD";
		return;
	}

	if (!checkVersion())
		return;

	checkQueryAndPath(target);

	if (!checkMethod())
		return;

	if (isChunkedTransfer()) {
		if (_client->statusCode() == 501)
			return;
		if (iEqual(_method, "POST")) {
			std::cout << getTimeStamp(_clientFd) << BLUE << "POST request with chunked body: " << RESET << _body.length() 
					<< " bytes of chunked data" << std::endl;
		}
	}
	_path = decode(_path);
}

void Request::setHeader(std::string& key, std::string& value) {
	if (iEqual(key, "Host")) {
		// if (_host.empty()) {
		_host = value;
		// }
		// else
			// _client->statusCode() = 400;
	}
	else if (iEqual(key, "Content-Type")) {
		// if (_contentType.empty()) {
		_contentType = value;
		// }
		// else
			// _client->statusCode() = 400;
	}
}

bool Request::checkMethod() {
	if (iEqual(_method, "GET") || iEqual(_method, "POST") || iEqual(_method, "DELETE")
		|| iEqual(_method, "PUT") || iEqual(_method, "HEAD") || iEqual(_method, "OPTIONS")
		|| iEqual(_method, "PATCH") || iEqual(_method, "TRACE") || iEqual(_method, "CONNECT")) {
		if (!iEqual(_method, "GET") && !iEqual(_method, "POST") && !iEqual(_method, "DELETE")) {
			_client->statusCode() = 501;
			_check = "NOTALLOWED";
			return false;
		}
	} else {
		_client->statusCode() = 400;
		_check = "BAD";
		return false;
	}
	return true;
}

bool Request::checkVersion() {
	if ( _version.empty() || _version != "HTTP/1.1") {
		_client->statusCode() = 505;
		_check = "BAD";
		return false;
	}
	return true;
}

void Request::checkQueryAndPath(std::string& target) {
	size_t queryPos = target.find('?');
	if (queryPos != std::string::npos) {
		_path = target.substr(0, queryPos);
		_query = target.substr(queryPos + 1);
	} else {
		_path = target;
		_query = "";
	}
	if ((_path == "/" || _path == "") && iEqual(_method, "GET")) {
		std::map<std::string, locationLevel>::iterator it = _curConf.locations.find("/");
		if (it != _curConf.locations.end())
			_path = matchAndAppendPath(it->second.rootLoc, it->second.indexFile);
	}

	size_t end = _path.find_last_not_of(" \t\r\n");
	if (end != std::string::npos)
		_path = _path.substr(0, end + 1);
}

void Request::parseHeaders(const std::string& headerSection) {
	std::istringstream iss(headerSection);
	std::string line;
	
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
			if (iEqual(key, "Host")) {
				std::vector<std::string> values = split(value);
				if (values.size() > 1) {
					_client->statusCode() = 400;
					continue;
				}
			}
			_headers[key] = value;
			setHeader(key, value);
		}
	}
	if (_host.empty())
		_client->statusCode() = 400;
}

void Request::checkContentLength(std::string buffer) {
	size_t pos = iFind(buffer, "Content-Length:");
	if (pos != std::string::npos) {
		pos += 15;
		size_t eol = buffer.find("\r\n", pos);
		if (eol == std::string::npos)
			eol = buffer.find("\n", pos);
		if (eol != std::string::npos) {
			std::string line = buffer.substr(pos, eol - pos);
			std::vector<std::string> values = split(line);
			if (values.size() > 1) {
				_client->statusCode() = 400;
				return;
			} else {
				_contentLength = strtoul(values[0].c_str(), NULL, 10);
				_hasLengthOrIsChunked = true;
			}
		} else {
			_client->statusCode() = 400;
			return;
		}
	}

	pos = iFind(buffer, "Transfer-Encoding:");
	if (pos != std::string::npos) {
		pos += 18;
		size_t eol = buffer.find("\r\n", pos);
		if (eol == std::string::npos)
			eol = buffer.find("\n", pos);
		if (eol != std::string::npos) {
			std::string transferEncoding = buffer.substr(pos, eol - pos);
			if (iFind(transferEncoding, "chunked") != std::string::npos) {
				_contentLength = 0;
				_hasLengthOrIsChunked = true;
				std::cout << getTimeStamp(_clientFd) << BLUE << "Transfer-Encoding: chunked detected" << RESET << std::endl;
			} else {
				_client->statusCode() = 501;
			}
		}
	}
}

void Request::parseContentType() {
	if (iFind(_contentType, "multipart/form-data") != std::string::npos) {
		size_t boundaryPos = iFind(_contentType, "boundary=");
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

bool Request::isChunkedTransfer() {
	std::map<std::string, std::string>::iterator it = iMapFind(_headers, "Transfer-Encoding");
	if (it != _headers.end()) {
		if (iFind(it->second, "chunked") != std::string::npos)
			return true;
		else {
			_client->statusCode() = 501;
			return false;
		}
	}
	return false;
}