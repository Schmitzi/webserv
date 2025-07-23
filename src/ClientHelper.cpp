#include "../include/ClientHelper.hpp"
#include "../include/Client.hpp"
#include "../include/Helper.hpp"
#include "../include/Response.hpp"
#include "../include/Server.hpp"
#include "../include/Request.hpp"

int checkLength(std::string& reqBuf, int fd, bool &printNewLine) {
	size_t contentLengthPos = iFind(reqBuf, "Content-Length:");
	if (contentLengthPos != std::string::npos) {
		size_t valueStart = contentLengthPos + 15;
		while (valueStart < reqBuf.length() && 
				(reqBuf[valueStart] == ' ' || reqBuf[valueStart] == '\t')) {
			valueStart++;
		}
		
		size_t valueEnd = reqBuf.find("\r\n", valueStart);
		if (valueEnd == std::string::npos) {
			valueEnd = reqBuf.find("\n", valueStart);
		}
		
		if (valueEnd != std::string::npos) {
			std::string lengthStr = reqBuf.substr(valueStart, valueEnd - valueStart);
			size_t expectedLength = strtoul(lengthStr.c_str(), NULL, 10);
			
			size_t bodyStart = reqBuf.find("\r\n\r\n");
			if (bodyStart != std::string::npos) {
				bodyStart += 4;
			} else {
				bodyStart = reqBuf.find("\n\n");
				if (bodyStart != std::string::npos) {
					bodyStart += 2;
				}
			}
			
			if (bodyStart != std::string::npos) {
				size_t actualBodyLength = reqBuf.length() - bodyStart;
				if (actualBodyLength < expectedLength) {
					if (printNewLine == false)
						std::cout << getTimeStamp(fd) << BLUE << "Receiving bytes..." << RESET << std::endl;
					std::cout << "[~]";
					printNewLine = true;
					return 0;
				}
			}
		}
	}
	return 1;
}

bool isChunkedBodyComplete(const std::string& buffer) {
	size_t bodyStart = buffer.find("\r\n\r\n");
	if (bodyStart == std::string::npos) {
		bodyStart = buffer.find("\n\n");
		if (bodyStart == std::string::npos)
			return false;
		bodyStart += 2;
	} else
		bodyStart += 4;
	
	if (bodyStart >= buffer.length())
		return false;
	
	std::string body = buffer.substr(bodyStart);
	
	if (body.find("0\r\n\r\n") != std::string::npos || body.find("0\n\n") != std::string::npos)
		return true;
	
	return false;
}

bool isFileBrowserRequest(const std::string& path) {
	return (path.length() >= 6 && path.substr(0, 6) == "/root/") || (path == "/root");
}

bool isCGIScript(const std::string& path) {
	size_t dotPos = path.find_last_of('.');
	
	if (dotPos != std::string::npos) {
		std::string ext = path.substr(dotPos + 1);
		
		static const char* whiteList[] = {"py", "php", "cgi", "pl", NULL};
		
		for (int i = 0; whiteList[i] != NULL; ++i) {
			if (ext.find(whiteList[i]) != std::string::npos)
				return true;
		}
	}
	return false;
}

int buildBody(Client& c, Request &req, std::string fullPath) {
	if (!tryLockFile(c, fullPath, c.getFd(), c.fileIsNew())) {
		req.setStatusCode(423);
		sendErrorResponse(c, req);
		return 1;
	}
	int fd = open(fullPath.c_str(), O_RDONLY);
	if (fd < 0) {
		releaseLockFile(fullPath);
		req.setStatusCode(500);
		c.output() = getTimeStamp(c.getFd()) + RED + "Failed to open file: " + RESET + fullPath;
		sendErrorResponse(c, req);
		return 1;
	}
	
	struct stat fileStat;
	if (fstat(fd, &fileStat) < 0) {
		translateErrorCode(errno, req.getStatusCode());
		releaseLockFile(fullPath);
		sendErrorResponse(c, req);
		close(fd);
		return 1;
	}
	
	std::vector<char> buffer(fileStat.st_size);
	ssize_t bytesRead = read(fd, buffer.data(), fileStat.st_size);
	close(fd);
	releaseLockFile(fullPath);
	if (bytesRead == 0)
		return 0;
	else if (bytesRead < 0) {
		c.output() = getTimeStamp(c.getFd()) + RED + "Error: read() failed on file: " + RESET + fullPath;
		req.setStatusCode(500);
		sendErrorResponse(c, req);
		return 1;
	}
	std::string fileContent(buffer.data(), bytesRead);
	req.setBody(fileContent);
	// if (req.getBody().size() != req.getContentLength()) {
	// 	req.setStatusCode(400);
	// 	req.check() = "BAD";
	// 	sendErrorResponse(c, req);
	// 	return 1;
	// }
	return 0;
}

bool ensureUploadDirectory(Client& c, Request& req) {
	struct stat st;
	std::string uploadDir = c.getServer().getUploadDir(c, req);
	if (stat(uploadDir.c_str(), &st) != 0) {
		req.setStatusCode(500);
		if (mkdir(uploadDir.c_str(), 0755) != 0) {
			c.output() = getTimeStamp(c.getFd()) + RED + "Error: Failed to create upload directory" + RESET;
			return false;
		}
	}
	return true;
}

bool isChunkedRequest(Request& req) {
	std::map<std::string, std::string> headers = req.getHeaders();
	std::map<std::string, std::string>::iterator it = headers.find("Transfer-Encoding");
	if (it != headers.end() && iFind(it->second, "chunked") != std::string::npos)
		return true;
	return false;
}

std::string getLocationPath(Client& c, Request& req, const std::string& method) {	
	locationLevel* loc = NULL;
	if (req.getPath().empty()) {
		req.setStatusCode(400);
		c.output() = getTimeStamp(c.getFd()) + RED + "Request path is empty for " + method + " request" + RESET;
		sendErrorResponse(c, req);
		return "";
	}
	if (!matchUploadLocation(req.getPath(), req.getConf(), loc)) {
		req.setStatusCode(404);
		c.output() = getTimeStamp(c.getFd()) + RED + "Location not found for " + method + " request: " + RESET + req.getPath();
		sendErrorResponse(c, req);
		return "";
	}
	for (size_t i = 0; i < loc->methods.size(); i++) {
		if (loc->methods[i] == method)
			break;
		if (i == loc->methods.size() - 1) {
			req.setStatusCode(405);
			c.output() = getTimeStamp(c.getFd()) + RED + "Method not allowed for " + method + " request: " + RESET + req.getPath();
			sendErrorResponse(c, req);
			return "";
		}
	}
	if (loc->uploadDirPath.empty()) {
		req.setStatusCode(403);
		c.output() = getTimeStamp(c.getFd()) + RED + "Upload directory not set for " + method + " request: " + RESET + req.getPath();
		sendErrorResponse(c, req);
		return "";
	}
	std::string fullPath = matchAndAppendPath(loc->rootLoc, loc->uploadDirPath);
	fullPath = matchAndAppendPath(fullPath, req.getPath());
	return fullPath;
}

std::string decodeChunkedBody(Client& c, int fd, const std::string& chunkedData) {
	std::string decodedBody;
	size_t pos = 0;
	
	while (pos < chunkedData.length()) {
		size_t crlfPos = chunkedData.find("\r\n", pos);
		size_t lineEndLength = 2;
		
		if (crlfPos == std::string::npos) {
			crlfPos = chunkedData.find("\n", pos);
			lineEndLength = 1;
			if (crlfPos == std::string::npos) {
				c.output() = getTimeStamp(fd) + RED  + "Malformed chunked data: no CRLF after chunk size" + RESET;
				break;
			}
		}
		
		std::string chunkSizeStr = chunkedData.substr(pos, crlfPos - pos);
		
		size_t semicolon = chunkSizeStr.find(';');
		if (semicolon != std::string::npos)
			chunkSizeStr = chunkSizeStr.substr(0, semicolon);
		
		chunkSizeStr.erase(0, chunkSizeStr.find_first_not_of(" \t"));
		chunkSizeStr.erase(chunkSizeStr.find_last_not_of(" \t") + 1);
		
		size_t chunkSize = 0;
		std::istringstream hexStream(chunkSizeStr);
		hexStream >> std::hex >> chunkSize;
		
		if (chunkSize == 0)
			break;
		
		pos = crlfPos + lineEndLength;
		
		if (pos + chunkSize > chunkedData.length())
			break;
		
		std::string chunkData = chunkedData.substr(pos, chunkSize);
		decodedBody += chunkData;
		
		pos += chunkSize;
		
		if (pos < chunkedData.length() && chunkedData[pos] == '\r')
			pos++;
		if (pos < chunkedData.length() && chunkedData[pos] == '\n')
			pos++;
	}
	
	std::cout << getTimeStamp(fd) << GREEN  << "Total decoded body: " << RESET << "\"" 
			<< decodedBody << "\" (" << decodedBody.length() << " bytes)" << std::endl;
	
	return decodedBody;
}

bool tryLockFile(Client& c, const std::string& path, int timeStampFd, bool& isNew) {
	int fd = open(path.c_str(), O_CREAT | O_EXCL, 0644);
	if (fd == -1 && errno == EEXIST)
		isNew = false;
	else {
		isNew = true;
		close(fd);
		remove(path.c_str());
	}
	
	std::string lockPath = path + ".lock";
	fd = open(lockPath.c_str(), O_CREAT | O_EXCL, 0644);
	if (fd == -1) {
		if (errno == EEXIST) {
			c.output() = getTimeStamp(timeStampFd) + RED + "Error: File is being used at the moment: " + RESET + path;
			return false;
		} else {
			c.output() = getTimeStamp(timeStampFd) + RED + "Error: creating lock file failed" + RESET;
			return false;
		}
	}
	close(fd);
	return true;
}

void releaseLockFile(const std::string& path) {
	std::string lockPath = path + ".lock";
	remove(lockPath.c_str());
}