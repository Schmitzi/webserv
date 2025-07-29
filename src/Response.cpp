#include "../include/Response.hpp"
#include "../include/ConfigParser.hpp"
#include "../include/Server.hpp"
#include "../include/Webserv.hpp"
#include "../include/Request.hpp"
#include "../include/Helper.hpp"
#include "../include/Client.hpp"
#include "../include/EpollHelper.hpp"

bool matchLocation(const std::string& path, const serverLevel& serv, locationLevel*& bestMatch) {
	size_t longestMatch = 0;
	bool found = false;

	std::map<std::string, locationLevel>::const_iterator it = serv.locations.begin();
	for (; it != serv.locations.end(); ++it) {
		if (it->second.isRegex) {
			size_t end = path.find_last_of(".");
			if (end != std::string::npos) {
				std::string ext = path.substr(end);
				if (iEqual(ext, it->first)) {
					bestMatch = const_cast<locationLevel*>(&(it->second));
					return true;
				}
			}
		} else {
			if (iFind(path, it->second.locName) != std::string::npos && it->second.locName.size() > longestMatch) {
				bestMatch = const_cast<locationLevel*>(&(it->second));
				found = true;
				longestMatch = it->second.locName.size();
			}
		}
	}
	return found;
}

bool matchUploadLocation(const std::string& path, const serverLevel& serv, locationLevel*& bestMatch) {
	size_t longestMatch = 0;
	bool found = false;
	std::map<std::string, locationLevel>::const_iterator it = serv.locations.begin();
	if (path.empty()) {
		for (; it != serv.locations.end(); ++it) {
			locationLevel* loc = const_cast<locationLevel*>(&(it->second));
			if (!loc->uploadDirPath.empty()) {
				bestMatch = loc;
				return true;
			}
		}
	} else {
		for (; it != serv.locations.end(); ++it) {
			locationLevel* loc = const_cast<locationLevel*>(&(it->second));
			if (loc->isRegex) {
				size_t end = path.find_last_of(".");
				if (end != std::string::npos) {
					std::string ext = path.substr(end);
					if (iEqual(ext, it->first)) {
						bestMatch = loc;
						return true;
					}
				}
			} else {
				if (iFind(path, loc->locName) != std::string::npos && loc->locName.size() > longestMatch) {
					bestMatch = loc;
					found = true;
					longestMatch = loc->locName.size();
				}
			}
		}
	}
	return found;
}

const std::string getStatusMessage(int code) {
	for (size_t i = 0; i < sizeof(httpErrors) / sizeof(httpErrors[0]); i++) {
		if (httpErrors[i].code == code)
			return httpErrors[i].message;
	}
	return "Unknown Status Code";
}

void generateErrorPage(std::string& body, int statusCode, const std::string& statusText) {
	body = "<!DOCTYPE html>\n"
			"<html>\n<head>\n"
			"<title>Error " + tostring(statusCode) + "</title>\n"
			"<style>\n"
			"  body { font-family: Arial, sans-serif; margin: 20px; line-height: 1.6; }\n"
			"  h1 { color: #D32F2F; }\n"
			"  .container { max-width: 800px; margin: 0 auto; padding: 20px; }\n"
			"  .server-info { font-size: 12px; color: #777; margin-top: 20px; }\n"
			"</style>\n"
			"</head>\n"
			"<body>\n"
			"<div class=\"container\">\n"
			"  <h1>Error " + tostring(statusCode) + " - " + statusText + "</h1>\n"
			"  <p>The server cannot process your request.</p>\n"
			"  <p><a href = \"/\">Return to homepage</a></p>\n"
			"  <div class=\"server-info\">\n"
			"    <p>WebServ/1.0</p>\n"
			"  </div>\n"
			"</div>\n"
			"</body>\n"
			"</html>";
}

std::string findErrorPage(int statusCode, const std::string& dir, Request& req) {
	std::map<std::vector<int>, std::string> errorPages = req.getConf().errPages;
	std::map<std::vector<int>, std::string>::iterator it = errorPages.begin();
	bool foundCustomPage = false;
	std::string uri;
	while (it != errorPages.end() && !foundCustomPage) {
		for (size_t i = 0; i < it->first.size(); i++) {
			if (it->first[i] == statusCode) {
				foundCustomPage = true;
				uri = it->second;
				break;
			}
		}
		++it;
	}
	
	std::string filePath;
	if (foundCustomPage) {
		if (uri.find(req.getConf().rootServ) == std::string::npos)
			filePath = matchAndAppendPath(req.getConf().rootServ, uri);
		else
			filePath = uri;
	}
	else
		filePath = matchAndAppendPath(dir, tostring(statusCode)) + ".html";
	return filePath;
}

void resolveErrorResponse(int statusCode, std::string& statusText, std::string& body, Request& req) {
	std::string dir = "errorPages";
	std::string filePath = findErrorPage(statusCode, dir, req);

	struct stat st;
	if (stat(dir.c_str(), &st) != 0) {
		mkdir(dir.c_str(), 0755);
	}
	std::ifstream file(filePath.c_str());
	if (file.good()) {
		std::stringstream buffer;
		buffer << file.rdbuf();
		body = buffer.str();
		file.close();
	} else {
		generateErrorPage(body, statusCode, statusText);
		std::ofstream out(filePath.c_str());
		if (out) {
			out << body;
			out.close();
		}
	}
}

void sendRedirect(Client& c, const std::string& location, Request& req) {
	std::string statusText = getStatusMessage(c.statusCode());
	
	std::string body = "<!DOCTYPE html>\n"
		"<html>\n"
		"<head>\n"
		"    <title>" + statusText + "</title></head>\n"
	    "    <body><h1>" + statusText + "</h1>\n"
	    "    <p>The document has moved <a href=\"" + location + "\">here</a>.</p>\n"
		"    </body>\n"
		"</html>\n";	
	
	std::string response = "HTTP/1.1 " + tostring(c.statusCode()) + " " + statusText + "\r\n";
	response += "Location: " + location + "\r\n";
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: " + tostring(body.size()) + "\r\n";
	response += "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n";
	response += "Pragma: no-cache\r\n";
	if (shouldCloseConnection(req))
		response += "Connection: close\r\n";
	response += "\r\n";
	response += body;
	response += "\n";
	addSendBuf(c.getWebserv(), c.getFd(), response);
	setEpollEvents(c.getWebserv(), c.getFd(), EPOLLOUT);
	c.output() = getTimeStamp(c.getFd()) + BLUE + "Sent redirect response: " + RESET + tostring(c.statusCode()) + " " + statusText + " to " + location;
}

ssize_t sendResponse(Client& c, Request& req, std::string body) {
	if (c.getFd() <= 0) {
		c.output() = getTimeStamp(c.getFd()) + RED + "Invalid fd in sendResponse" + RESET;
		return -1;
	}
	std::string response = "HTTP/1.1 " + tostring(c.statusCode()) + " OK\r\n";
	response += "Content-Type: " + req.getContentType() + "\r\n";
	
	std::string content = body;
	
	if (iEqual(req.getMethod(), "POST") && content.empty())
		content = "Upload successful";
	
	if (!req.isChunked())
		response += "Content-Length: " + tostring(content.size()) + "\r\n";
	else
		response += "Transfer-Encoding: chunked\r\n";
	
	response += "Server: WebServ/1.0\r\n";
	
	if (shouldCloseConnection(req))
		response += "Connection: close\r\n";
	response += "Access-Control-Allow-Origin: *\r\n";
	response += "\r\n";
	
	if (c.getFd() < 0) {
		c.output() = getTimeStamp(c.getFd()) + RED  + "FD became invalid before send" + RESET;
		return -1;
	}
	addSendBuf(c.getWebserv(), c.getFd(), response);
	if (!content.empty()) {
		if (req.isChunked()) {
			const size_t chunkSize = 4096;
			size_t remaining = content.length();
			size_t offset = 0;
			
			while (remaining > 0) {
				size_t currentChunkSize;
				if (remaining < chunkSize) 
					currentChunkSize = remaining;
				else 
					currentChunkSize = chunkSize;
				
				std::stringstream hexStream;
				hexStream << std::hex << currentChunkSize;
				std::string all = hexStream.str() + "\r\n";
				all += content.c_str() + offset;
				all += "\r\n";
				addSendBuf(c.getWebserv(), c.getFd(), all);
				offset += currentChunkSize;
				remaining -= currentChunkSize;
			}
			
			std::string s2 = "0\r\n\r\n";
			addSendBuf(c.getWebserv(), c.getFd(), s2);
			setEpollEvents(c.getWebserv(), c.getFd(), EPOLLOUT);
			c.output() = getTimeStamp(c.getFd()) + GREEN  + "Sent chunked body " + RESET + "(" + tostring(content.length()) + " bytes)";
			return 0;
		} else {
			content += "\n";
			addSendBuf(c.getWebserv(), c.getFd(), content);
			setEpollEvents(c.getWebserv(), c.getFd(), EPOLLOUT);
			return 0;
		}
	} else {
		addSendBuf(c.getWebserv(), c.getFd(), "\n");
		setEpollEvents(c.getWebserv(), c.getFd(), EPOLLOUT);
		c.output() = getTimeStamp(c.getFd()) + GREEN  + "Response sent (headers only)" + RESET;
		return 0;
	}
}

void sendErrorResponse(Client& c, Request& req) {
	std::string body;
	std::string statusText = getStatusMessage(c.statusCode());
	
	resolveErrorResponse(c.statusCode(), statusText, body, req);    

	std::string response = "HTTP/1.1 " + tostring(c.statusCode()) + " " + statusText + "\r\n";
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: " + tostring(body.size()) + "\r\n";
	response += "Server: WebServ/1.0\r\n";
	response += "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n";
	response += "Pragma: no-cache\r\n";
	if (shouldCloseConnection(req))
		response += "Connection: close\r\n";
	response += "\r\n";
	response += body;
	response += "\n";
	addSendBuf(c.getWebserv(), c.getFd(), response);
	setEpollEvents(c.getWebserv(), c.getFd(), EPOLLOUT);
	c.output() = getTimeStamp(c.getFd()) + RED  + "Error sent: " + tostring(c.statusCode()) + " " + getStatusMessage(c.statusCode()) + RESET;
}

bool shouldCloseConnection(Request& req) {
	std::map<std::string, std::string>::iterator it = iMapFind(req.getHeaders(), "Connection");
	if (it != req.getHeaders().end() && iEqual(it->second, "close")) {
		std::cout << CYAN << "HERE 3" << RESET << std::endl;
		req.getClient().shouldClose() = true;
		return true;
	}

	if (!req.isChunked() && !req.hasValidLength()) {//TODO: add a better check for hasValidLength!
		std::cout << CYAN << "HERE 6" << RESET << std::endl;
		req.getClient().shouldClose() = true;
		return true;
	}

	// if (req.getClient().shouldClose() == true) {
	// 	std::cout << CYAN << "HERE 1" << RESET << std::endl;
	// 	return true;
	// }

	if (req.getClient().statusCode() >= 400) {
		if (req.getClient().statusCode() == 413 || req.getClient().statusCode() == 501) {
			std::cout << CYAN << "HERE 4" << RESET << std::endl;
			req.getClient().shouldClose() = false;
			return false;
		}
		std::cout << CYAN << "HERE 5" << RESET << std::endl;
		req.getClient().shouldClose() = true;
		return true;
	}

	if (req.getClient().statusCode() < 400) {
		std::cout << CYAN << "HERE 2" << RESET << std::endl;
		req.getClient().shouldClose() = false;
		return false;
	}

	std::cout << CYAN << "HERE 7" << RESET << std::endl;
	req.getClient().shouldClose() = false;
	return false;
}

void translateErrorCode(int errnoCode, int& statusCode) {
	switch (errnoCode) {
		case ENOENT:
			statusCode = 404;
			return;
		case EACCES:
			statusCode = 403;
			return;
		case ENOTDIR:
			statusCode = 404;
			return;
		case EISDIR:
			statusCode = 403;
			return;
		case ENOSPC:
			statusCode = 507;
			return;
		case ENAMETOOLONG:
			statusCode = 414;
			return;
		case EINVAL:
			statusCode = 400;
			return;
		case EIO:
			statusCode = 500;
			return;
		case EPERM:
			statusCode = 403;
			return;
		case EFAULT:
			statusCode = 500;
			return;
		default:
			statusCode = 500;
			return;
	}
}