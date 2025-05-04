#include "../include/Request.hpp"

Request::Request() {
    _headers._method = "GET";
    _headers._path = "/";
    _headers._contentType = "text/html";
    _headers._version = "HTTP/1.1";
    _headers._body = "";
    _headers._query = "";
    _headers._boundary = ""; 
}


Request::Request(std::string const buffer) {
    std::vector<std::string> header = split(buffer, '\n');
    std::map<std::string, std::string> temp = mapSplit(header);
    std::vector<std::string> request = split(header[0], ' ');
    if (!request[0].empty()) 
        _headers._method = request[0];
    else
        _headers._method = "GET";
    
    if (!request[1].empty())  {
        if (request[1].find("?") != std::string::npos) {
            _headers._path = request[1].substr(0, request[1].find("?"));
            _headers._query = request[1].substr(request[1].find("?"), request[1].length());
        } else {
            _headers._path = request[1];
            _headers._query = "";
        }
    } else {
        _headers._path = "/";
    }
    if (!request[2].empty()) {
        _headers._version = request[2];
    } else {
        _headers._version = "HTTP/1.1";
    }
    for (size_t i = 0; i < header.size(); i++) {
        //  "Content-Type: multipart/form-data"
        if (header[i].substr(0, 14) == "Content-Type: ") {
            if (header[i].find("boundary=") != std::string::npos) {
                _headers._boundary = header[i].substr(45, header[i].length());
                _headers._contentType = header[i].substr(14, 34);
            } else {
                _headers._boundary = "";
                _headers._contentType = header[i].substr(14, header[i].length());
            }
            break ;
        }
    }
    _headers._body = "";
    _headers._contentDisp = temp["Content-Disposition"];
}

Request::~Request() {

}

std::string const &Request::getPath() {
    return _headers._path;
}

std::string const &Request::getMethod() {
    return _headers._method;
}

std::string const &Request::getVersion() {
    return _headers._version;
}

std::string const &Request::getBody() {
    return _headers._body;
}

std::string const &Request::getContentType() {
    return _headers._contentType;
}

std::string const &Request::getQuery() {
    return _headers._query;
}

std::string const &Request::getBoundary() {
    return _headers._boundary;
}

std::string const &Request::getContentDisp() {
    return _headers._contentDisp;
}

// std::map<std::string, std::string>::getHeaders() {
//     return _headers;
// }

void    Request::setMethod(std::string const method) {
    _headers._method = method;
}

void    Request::setPath(std::string const path) {
    _headers._path = path;
}

void    Request::setVersion(std::string const version) {
    _headers._version = version;
}

void    Request::setBody(std::string const body) {
    _headers._body = body;
}

void    Request::setQuery(std::string const query) {
    _headers._query = query;
}

void    Request::setContentType(std::string const content) {
    _headers._contentType = content;
}

void    Request::setBoundary(std::string boundary) {
    _headers._boundary = boundary;
}

void    Request::setHeader(std::map<std::string, std::string> map) {
    (void)map;
    // _headers = map;

}

void    Request::setFileName(std::string const filename) {
    _headers._fileName = filename;
}

void Request::formatPost(std::string const target) {  
    setMethod("POST");
    setVersion("HTTP/1.1");
    size_t queryPos = target.find('?');
    if (queryPos != std::string::npos) {
        setPath(target.substr(0, queryPos));
        setQuery(target.substr(queryPos + 1));
    } else {
        setPath(target);
        setQuery("");
    }
}

void    Request::formatDelete(std::string const token) {
    setMethod("DELETE");
    setVersion("HTTP/1.1");
    setPath("upload/" + token);
}

int Request::formatGet(std::string const token) {
    setMethod("GET");
    setVersion("HTTP/1.1");
    
    size_t queryPos = token.find('?');
    if (queryPos != std::string::npos) {
        setPath(token.substr(0, queryPos));
        setQuery(token.substr(queryPos + 1));
    } else {
        setPath(token);
        setQuery("");
    }
    
    return 0;
}

std::string Request::getMimeType(std::string const &path) {
    std::string ext;
    size_t dotPos = path.find_last_of(".");
    
    if (dotPos != std::string::npos) {
        ext = path.substr(dotPos + 1);
    } else if (path == "/") {
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
        return "image/x-icon";
    
    return "text/plain"; // Default
}

void Request::buildBuffer() {
    
}
