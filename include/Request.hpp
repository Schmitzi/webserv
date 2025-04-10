#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <iostream>
#include <unistd.h>
#include <map>

class Request {
    public:
        Request();
        ~Request();
        std::string const &getPath();
        std::string const &getMethod();
        std::string const &getVersion();
        std::string const &getBody();
        std::string const &getContentType();
        std::string const &getQuery();
        void    setMethod(std::string const method);
        void    setPath(std::string const path);
        void    setVersion(std::string const version);
        void    setBody(std::string const body);
        void    setQuery(std::string const query);
        void    setContentType(std::string const content);
        void    formatPost(std::string const target);
        void    formatDelete(std::string const token);
        int     formatGet(std::string const token);
        std::string getMimeType(std::string const &path);
    private:
        std::string                         _method;
        std::string                         _path;
        std::string                         _contentType;
        std::string                         _version;
        std::map<std::string, std::string>  _headers;
        std::string                         _body;
        std::string                         _query;
};

#endif