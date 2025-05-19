#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <iostream>
#include <unistd.h>
#include <map>
#include <sstream>
#include "../include/ConfigParser.hpp"

struct serverLevel;

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
        void    setMethod(std::string const method);
        void    setPath(std::string const path);
        void    setVersion(std::string const version);
        void    setBody(std::string const body);
        void    setQuery(std::string const query);
        void    setContentType(std::string const content);
        void    setBoundary(std::string boundary);
        void    setHeader(std::map<std::string, std::string> map);
        void    formatPost(std::string const target);
        void    formatDelete(std::string const token);
        int     formatGet(std::string const token);
        void    parse(const std::string& rawRequest);
        void    parseHeaders(const std::string& headerSection);
        void    checkContentLength();
        void    parseContentType();
        bool    isChunkedTransfer() const;
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