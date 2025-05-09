#ifndef MULTIPART_HPP
#define MULTIPART_HPP

#include <iostream>

class Multipart {
    public:
        Multipart(const std::string& data, const std::string& boundary);
        
        bool parse();
        std::string getFilename() const;
        std::string getFileContent() const;
        bool isComplete() const;
    private:
        size_t findPartStart(size_t boundaryPos, const std::string& fullBoundary);
        size_t findHeadersEnd(size_t partStart);
        bool parseHeaders(size_t partStart, size_t headersEnd);
        bool extractFilename(const std::string& headers);
        size_t findLineEnd(const std::string& text, size_t start);
        bool parseContent(size_t contentStart, const std::string& fullBoundary);

        const std::string& _data;
        std::string _boundary;
        std::string _filename;
        std::string _fileContent;
        bool _isComplete;
};

#endif       