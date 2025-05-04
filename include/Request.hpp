#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <iostream>
#include <unistd.h>
#include <map>
#include <string>
#include <sstream>
#include <algorithm>

class Request {
    public:
        Request();
        Request(const std::string& rawRequest);
        ~Request();
        std::string const &getPath();
        std::string const &getMethod();
        std::string const &getVersion();
        std::string const &getBody();
        std::string const &getContentType();
        std::string const &getQuery();
        std::string const &getBoundary();
        std::map<std::string, std::string> &getHeaders();
        std::string getMimeType(std::string const &path);
        void    setBody(std::string const body);
        void    setContentType(std::string const content);
        void    setHeader(std::map<std::string, std::string> map);
        void    parse(const std::string& rawRequest);
        void    parseHeaders(const std::string& headerSection);
        void    parseContentType();
        void    buildBuffer();
    private:
        std::string                         _method;
        std::string                         _path;
        std::string                         _contentType;
        std::string                         _version;
        std::map<std::string, std::string>  _headers;
        std::string                         _body;
        std::string                         _query;
        std::string                         _boundary;
};

#endif