#include "../include/Request.hpp"

Request::Request() {
    
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

void    Request::formatPost(std::string const target) {
    setMethod("POST");
    setVersion("HTTP/1.1");
    
    if (target.find("[") != std::string::npos) {
        setPath(target.substr((target.find(" ") + 1), target.find("[") - 5));
        
        size_t queryStart = target.find("\"?\"") + 3;
        if (queryStart != std::string::npos) {
            size_t bodyStart = queryStart;
            size_t bodyEnd = target.find("]", bodyStart);
            
            if (bodyEnd != std::string::npos) {
                setBody(target.substr(bodyStart, bodyEnd - bodyStart));
            }
        }
    } else {
        setPath(target.substr(target.find(" ") + 1, target.length()));
    }
}

void    Request::formatDelete(std::string const token) {
    setMethod("DELETE");
    setPath("upload/" + token);
    setVersion("HTTP/1.1");
}

void    Request::formatGet(std::string const token) {
    setMethod("GET");
    setPath(token);
    setVersion("HTTP/1.1");
}