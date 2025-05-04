#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <iostream>
#include <unistd.h>
#include <map>
#include "../include/Helper.hpp"

#define BLUE    "\33[34m"
#define GREEN   "\33[32m"
#define RED     "\33[31m"
#define WHITE   "\33[97m"
#define RESET   "\33[0m" // No Colour

typedef struct s_header {
    std::string			_method;
	std::string			_path;
    std::string         _contentType;
    std::string			_version;
	std::string			_body;
	std::string			_query;
	std::string			_boundary;
    std::string         _contentDisp;
    std::string         _fileName;
} t_header;

class Request {
    public:
        Request();
		Request(std::string const buffer);
        ~Request();
        std::string const &getPath();
        std::string const &getMethod();
        std::string const &getVersion();
        std::string const &getBody();
        std::string const &getContentType();
        std::string const &getQuery();
        std::string const &getBoundary();
        std::string const &getContentDisp();
        // std::map<std::string, std::string> &getHeaders();
        std::string getMimeType(std::string const &path);
        void    setMethod(std::string const method);
        void    setPath(std::string const path);
        void    setVersion(std::string const version);
        void    setBody(std::string const body);
        void    setQuery(std::string const query);
        void    setContentType(std::string const content);
        void    setBoundary(std::string boundary);
        void    setFileName(std::string const filename)
;        void    setHeader(std::map<std::string, std::string> map);
        void    formatPost(std::string const target);
        void    formatDelete(std::string const token);
        int     formatGet(std::string const token);
        void    buildBuffer();
    private:
        // std::string                         _method;
        // std::string                         _path;
        // std::string                         _contentType;
        // std::string                         _version;
        // std::map<std::string, std::string>  _headers;
		t_header							_headers;
        // std::string                         _body;
        // std::string                         _query;
        // std::string                         _boundary;
};

#endif