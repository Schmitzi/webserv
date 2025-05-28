#include "../include/Request.hpp"

Request::Request() {}

Request::Request(const std::string& rawRequest, serverLevel& conf) : 
    _method("GET"),
    _path(""),
    _contentType(""),
    _version(""),
    _headers(),
    _body(""),
    _query(""),
    _boundary(""),
	_reqPath(""),
	_contentLength(0),
	_conf(conf)
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

serverLevel& Request::getConf() {
	return _conf;
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
    std::cout << "REQ: \n" << rawRequest << "\n";
    if (rawRequest.empty()) {
        std::cout << RED << "Empty request!\n" << RESET;
        _method = "BAD";
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
    parseContentType();

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

    if (_method.empty() || (_method != "GET" && target.empty()) || _version.empty()) {
        _method = "BAD";
        std::cout << RED << "Invalid request line!\n" << RESET;
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
    if (reqPathEnd != std::string::npos)
        _reqPath = _path.substr(0, reqPathEnd + 1);
    
    if (_method != "GET" && _method != "POST" && _method != "DELETE" && 
        _method != "HEAD" && _method != "OPTIONS") {
        _method = "BAD";
        return;
    }

    if (isChunkedTransfer()) {
        std::cout << BLUE << getTimeStamp() << "Chunked transfer encoding detected" << RESET << std::endl;
        if (_method == "POST") {
            std::cout << BLUE << getTimeStamp() << "POST request with chunked body: " << _body.length() 
                      << " bytes of chunked data" << RESET << std::endl;
        }
    }
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
    size_t pos = buffer.find("Content-Length:");
    if (pos != std::string::npos) {
        pos += 15;
        while (pos < buffer.size() && (buffer[pos] == ' ' || buffer[pos] == '\t')) {
            pos++;
        }
        
        size_t eol = buffer.find("\r\n", pos);
        if (eol == std::string::npos) {
            eol = buffer.find("\n", pos);
        }
        
        if (eol != std::string::npos) {
            std::string valueStr = buffer.substr(pos, eol - pos);
            _contentLength = strtoul(valueStr.c_str(), NULL, 10);
            return;
        }
    }
    
    pos = buffer.find("Transfer-Encoding:");
    if (pos != std::string::npos) {
        size_t eol = buffer.find("\r\n", pos);
        if (eol == std::string::npos) {
            eol = buffer.find("\n", pos);
        }
        
        if (eol != std::string::npos) {
            std::string transferEncoding = buffer.substr(pos + 18, eol - pos - 18);
            if (transferEncoding.find("chunked") != std::string::npos) {
                _contentLength = 0;
                std::cout << BLUE << getTimeStamp() << "Transfer-Encoding: chunked detected" << RESET << std::endl;
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
    } else if (_method == "POST") {
        _contentType = "application/octet-stream";
    }
}

std::string Request::getMimeType(std::string const &path) {
    std::string ext;
    size_t dotPos = path.find_last_of(".");
    
    if (dotPos != std::string::npos) {
        ext = path.substr(dotPos + 1);
        
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    } else if (path == "/" || path.empty()) {
        return "text/html";
    } else {
        return "text/plain";
    }
    
    // CRITICAL FIX: Proper MIME type mapping
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
    
    return "application/octet-stream"; // Default for unknown files
}

bool Request::isChunkedTransfer() const {
    std::map<std::string, std::string>::const_iterator it = _headers.find("Transfer-Encoding");
    if (it != _headers.end() && it->second.find("chunked") != std::string::npos) {
        return true;
    }
    return false;
}

std::string Request::getTimeStamp() {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    std::ostringstream oss;
    oss << "[" 
        << (tm_info->tm_year + 1900) << "-"
        << std::setw(2) << std::setfill('0') << (tm_info->tm_mon + 1) << "-"
        << std::setw(2) << std::setfill('0') << tm_info->tm_mday << " "
        << std::setw(2) << std::setfill('0') << tm_info->tm_hour << ":"
        << std::setw(2) << std::setfill('0') << tm_info->tm_min << ":"
        << std::setw(2) << std::setfill('0') << tm_info->tm_sec << "] ";
    
    return oss.str();
}