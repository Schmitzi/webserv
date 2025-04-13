#include "../include/Request.hpp"

Request::Request() : _method("GET"), _path("/"), _version("HTTP/1.1"), _body("") {
}

Request::~Request() {

}

std::string const &Request::getPath() {
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
