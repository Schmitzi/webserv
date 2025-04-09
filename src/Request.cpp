#include "../include/Request.hpp"

Request::Request() : _method("GET"), _path("/index"), _version("HTTP/1.1") {
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

int    Request::formatGet(std::string const token) {
    setMethod("GET");
    if (!token.empty())
        setPath(token);
    else
        return 1;
    setVersion("HTTP/1.1");
    return 0;
}

std::string Request::getMimeType(std::string const &path) {
    std::string ext;
    size_t dotPos = path.find_last_of(".");
    
    if (dotPos != std::string::npos) {
        ext = path.substr(dotPos + 1);
    } else {
        return "text/html";
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