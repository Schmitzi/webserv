#include "../include/Multipart.hpp"

Multipart::Multipart(const std::string& data, const std::string& boundary) 
            : _data(data), _boundary(boundary), _isComplete(false) {}

bool Multipart::parse() {
    std::string fullBoundary = "--" + _boundary;
    
    size_t boundaryPos = _data.find(fullBoundary);
    if (boundaryPos == std::string::npos) {
        return false;
    }
    
    size_t partStart = findPartStart(boundaryPos, fullBoundary);
    size_t headersEnd = findHeadersEnd(partStart);
    
    if (headersEnd == std::string::npos) {
        return false;
    }
    
    if (!parseHeaders(partStart, headersEnd)) {
        return false;
    }
    
    return parseContent(headersEnd, fullBoundary);
}

std::string Multipart::getFilename() const { 
    return _filename; 
}

std::string Multipart::getFileContent() const { 
    return _fileContent; 
}

bool Multipart::isComplete() const { 
    return _isComplete; 
}

size_t Multipart::findPartStart(size_t boundaryPos, const std::string& fullBoundary) {
    size_t partStart = boundaryPos + fullBoundary.length();
    if (partStart + 2 < _data.length() && 
        _data[partStart] == '\r' && _data[partStart + 1] == '\n') {
        partStart += 2;
    }
    return partStart;
}

size_t Multipart::findHeadersEnd(size_t partStart) {
    size_t headersEnd = _data.find("\r\n\r\n", partStart);
    if (headersEnd == std::string::npos) {
        headersEnd = _data.find("\n\n", partStart);
        if (headersEnd != std::string::npos) {
            headersEnd += 2;
        }
    } else {
        headersEnd += 4;
    }
    return headersEnd;
}

bool Multipart::parseHeaders(size_t partStart, size_t headersEnd) {
    std::string partHeaders = _data.substr(partStart, headersEnd - partStart);
    return extractFilename(partHeaders);
}

bool Multipart::extractFilename(const std::string& headers) {
    size_t cdPos = headers.find("Content-Disposition:");
    if (cdPos == std::string::npos) {
        return false;
    }
    
    size_t cdEnd = findLineEnd(headers, cdPos);
    std::string contentDisposition = headers.substr(cdPos, cdEnd - cdPos);
    
    size_t filenamePos = contentDisposition.find("filename=\"");
    if (filenamePos == std::string::npos) {
        return false;
    }
    
    filenamePos += 10;
    size_t filenameEnd = contentDisposition.find("\"", filenamePos);
    
    if (filenameEnd == std::string::npos) {
        return false;
    }
    
    _filename = contentDisposition.substr(filenamePos, filenameEnd - filenamePos);
    return true;
}

size_t Multipart::findLineEnd(const std::string& text, size_t start) {
    size_t end = text.find("\r\n", start);
    if (end == std::string::npos) {
        end = text.find("\n", start);
    }
    if (end == std::string::npos) {
        end = text.length();
    }
    return end;
}

bool Multipart::parseContent(size_t contentStart, const std::string& fullBoundary) {
    std::string endBoundary = "\r\n" + fullBoundary + "--";
    size_t contentEnd = _data.find(endBoundary, contentStart);
    
    if (contentEnd == std::string::npos) {
        endBoundary = "\n" + fullBoundary + "--";
        contentEnd = _data.find(endBoundary, contentStart);
    }
    
    if (contentEnd == std::string::npos) {
        _isComplete = false;
        return true; // Partial upload
    }
    
    _fileContent = _data.substr(contentStart, contentEnd - contentStart);
    _isComplete = true;
    return true;
}