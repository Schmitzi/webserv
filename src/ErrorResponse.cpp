#include "../include/ErrorResponse.hpp"

const struct locationLevel* matchLocation(const std::string& uri, const struct serverLevel& serv) {
	const struct locationLevel* bestMatch = NULL;
	size_t longestMatch = 0;

	std::map<std::string, struct locationLevel>::const_iterator it = serv.locations.begin();
	for (; it != serv.locations.end(); ++it) {
		const struct locationLevel& loc = it->second;
		if (uri.find(loc.locName) == 0 && loc.locName.size() > longestMatch) {
			bestMatch = &loc;
			longestMatch = loc.locName.size();
		}
	}
	return bestMatch;
}


std::string resolveFilePathFromUri(const std::string& uri, const struct serverLevel& serv) {
	const struct locationLevel* loc = matchLocation(uri, serv);
	if (!loc)
		return "/" + uri;
	std::string root = loc->rootLoc;
	std::string locationPath = loc->locName;
	std::string relativeUri = uri.substr(locationPath.size());
	if (!root.empty() && root[root.size() - 1] == '/' && !relativeUri.empty() && relativeUri[0] == '/') {
		relativeUri = relativeUri.substr(1);
	}
	return root + "/" + relativeUri;
}

const std::string getStatusMessage(int code) {
	for (size_t i = 0; i < sizeof(httpErrors) / sizeof(httpErrors[0]); i++) {
		if (httpErrors[i].code == code)
			return httpErrors[i].message;
	}
	return "Unknown Status Code";
}

void resolveErrorResponse(int statusCode, Webserv& webserv, std::string& statusText, std::string& body) {
	std::string uri;
	std::string dir = "errorPages";
	std::map<std::vector<int>, std::string> errorPages = webserv.getConfig().getConfig().errPages;
	std::map<std::vector<int>, std::string>::iterator it = errorPages.begin();
	for (; it != errorPages.end(); ++it) {
		if (it->first[0] == statusCode)
			break;
	}
	std::string filePath;
	if (it != errorPages.end()) {
		uri = it->second;
		filePath = resolveFilePathFromUri(uri, webserv.getConfig().getConfig());
	} else
		filePath = dir + "/" + tostring(statusCode) + ".html";
	struct stat st;
	if (stat(dir.c_str(), &st) != 0)
		mkdir(dir.c_str(), 0755);
	std::ifstream file(filePath.c_str());
	if (file) {
		std::stringstream buffer;
		buffer << file.rdbuf();
		body = buffer.str();
		file.close();
	} else {
		body = "<!DOCTYPE html>\n"
			"<html>\n<head><title>Error " + tostring(statusCode) + "</title></head>\n"
			"<body>\n<h1>" + statusText + "</h1>\n"
			"<p>The server encountered an error: " + statusText + " (" + tostring(statusCode) + ")</p>\n"
			"<hr>\n<em>WebServ/1.0</em>\n"
			"</body>\n</html>";
		std::ofstream out(filePath.c_str());
		if (out) {
			out << body;
			out.close();
		}
	}
}