#include "../include/Request.hpp"

Request::Request() : _method("GET"), _path("/"), _version("HTTP/1.1"), _body("") {
}

Request::Request(const std::string& rawRequest) : 
    _method("GET"),
    _path("/"),
    _contentType(""),
    _version("HTTP/1.1"),
    _headers(),
    _body(""),
    _query(""),
    _boundary("")
{
    parse(rawRequest);
}

Request::~Request() {

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

std::map<std::string, std::string> &Request::getHeaders() {
    return _headers;
}

std::string Request::getReqPath() const {
	return _reqPath;
}

size_t         &Request::getContentLength() {
    return _contentLength;
}

void    Request::setMethod(std::string const method) {
    _method = method;
}

void    Request::setPath(std::string const path) {
    _path = path;
}

void    Request::setVersion(std::string const version) {
    _version = version;
}

void    Request::setBody(std::string const body) {
    _body = body;
}

void    Request::setQuery(std::string const query) {
    _query = query;
}

void    Request::setContentType(std::string const content) {
    _contentType = content;
}

void    Request::setBoundary(std::string boundary) {
    _boundary = boundary;
}

void    Request::setHeader(std::map<std::string, std::string> map) {
    _headers = map;
}

void Request::parse(const std::string& rawRequest) {
    if (rawRequest.empty()) {
        _method = "BAD";
        return;
    }

    checkContentLength(rawRequest);

    // Find headers and body
    size_t headerEnd = rawRequest.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = rawRequest.find("\n\n");
        if (headerEnd == std::string::npos) {
            headerEnd = rawRequest.length();
        } else {
            headerEnd += 2;
        }
    } else {
        headerEnd += 4;
    }

    std::string headerSection = rawRequest.substr(0, headerEnd);
    if (headerEnd < rawRequest.length()) {
        _body = rawRequest.substr(headerEnd);
    }

    std::istringstream iss(headerSection);
    std::string requestLine;
    std::getline(iss, requestLine);

	size_t end = requestLine.find_last_not_of(" \t\r\n");
    if (end != std::string::npos) {
        requestLine = requestLine.substr(0, end + 1);
    }

    std::istringstream lineStream(requestLine);
    std::string target;
    lineStream >> _method >> target >> _version;

    if (_method.empty() || (_method != "GET" && target.empty())) {
        _method = "BAD";
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
        _path = "/index.html";
    }

    end = _path.find_last_not_of(" \t\r\n");
    if (end != std::string::npos) {
        _path = _path.substr(0, end + 1);
    }
	size_t reqPathEnd = _path.find_last_of("/");
	if (reqPathEnd != std::string::npos) {
		_reqPath = _path.substr(0, reqPathEnd + 1);
	} else {
		_reqPath = "";
	}
    if (_method != "GET" && _method != "POST" && _method != "DELETE") {
        _method = "BAD";
        return;
    }

    parseHeaders(headerSection);
    parseContentType();
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
            
            _headers[key] = value;
        }
    }
}

void Request::checkContentLength(std::string buffer) {
    size_t pos = buffer.find("Content-Length:");;
    if (pos != std::string::npos) {
        pos += 15;

        while (pos < buffer.size() && (buffer[pos] == ' ' || buffer[pos] == '\t')) {
            pos++;
        }
        
        size_t eol = buffer.find("\r\n", pos);
        
        if (eol != std::string::npos) {
            std::string valueStr = buffer.substr(pos, eol - pos);
            _contentLength = strtoul(valueStr.c_str(), NULL, 10);
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
    } else if (_method == "POST") {
        _contentType = "application/octet-stream";
    }
}

std::string Request::getMimeType(std::string const &path) {
    std::string ext;
    size_t dotPos = path.find_last_of(".");
    
    if (dotPos != std::string::npos) {
        ext = path.substr(dotPos + 1);
    } else if (path == "/" || path.empty()) {
        return "text/html";
    } else {
        return "text/plain";
    }
    
    if (ext == "html" || ext == "htm")
        return "text/html";
    if (ext == "css")
        return "text/css";
    if (ext == "js") 
        return "application/javascript";
    if (ext == "jpg" || ext == "jpeg") 
        return "image/jpeg";
    if (ext == "png")
        return "image/png";
    if (ext == "gif")
        return "image/gif";
    if (ext == "svg")
        return "image/svg+xml";
    if (ext == "ico")
        return "image/vnd.microsoft.icon";
    
    return "text/plain"; // Default
}

bool Request::isChunkedTransfer() const {
    std::map<std::string, std::string>::const_iterator it = _headers.find("Transfer-Encoding");
    if (it != _headers.end() && it->second.find("chunked") != std::string::npos) {
        return true;
    }
    return false;
}