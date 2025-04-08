#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <iostream>
#include <map>

class Request {
    public:
        Request();
        ~Request();
        std::string const &getPath();
        std::string const &getMethod();
        std::string const &getVersion();
        std::string const &getBody();
        void    setMethod(std::string const method);
        void    setPath(std::string const path);
        void    setVersion(std::string const version);
        void    setBody(std::string const body);
        void    formatPost(std::string const target);
        void    formatDelete(std::string const token);
        void    formatGet(std::string const token);
    private:
        std::string                         _method;
        std::string                         _path;
        std::string                         _version;
        std::map<std::string, std::string>  _headers;
        std::string                         _body;
};

#endif