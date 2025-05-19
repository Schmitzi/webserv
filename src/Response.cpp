#include "../include/Response.hpp"
#include "../include/Config.hpp"

bool matchLocation(const std::string& path, const serverLevel& serv, locationLevel& bestMatch) {
	size_t longestMatch = 0;
	bool found = false;
	std::map<std::string, locationLevel>::const_iterator it = serv.locations.begin();
	for (; it != serv.locations.end(); ++it) {
		locationLevel loc = it->second;
		if (path.find(loc.locName) == 0 && loc.locName.size() > longestMatch) {
			bestMatch = loc;
			found = true;
			longestMatch = loc.locName.size();
		}
	}
	return found;
}

bool matchUploadLocation(const std::string& path, const serverLevel& serv, locationLevel& bestMatch) {
	size_t longestMatch = 0;
	bool found = false;
	std::map<std::string, locationLevel>::const_iterator it = serv.locations.begin();
	if (path.empty()) {
		for (; it != serv.locations.end(); ++it) {
			locationLevel loc = it->second;
			if (!loc.uploadDirPath.empty()) {
				bestMatch = loc;
				return true;
			}
		}
	} else {
		for (; it != serv.locations.end(); ++it) {
			locationLevel loc = it->second;
			if (path.find(loc.locName) == 0 && loc.locName.size() > longestMatch) {
				bestMatch = loc;
				found = true;
				longestMatch = loc.locName.size();
			}
		}
	}
	return found;
}

bool matchRootLocation(const std::string& uri, serverLevel& serv, locationLevel& bestMatch) {
	bool found = false;
	size_t longestMatch = 0;

	std::map<std::string, locationLevel>::iterator it = serv.locations.begin();
	for (; it != serv.locations.end(); ++it) {
		locationLevel loc = it->second;
		if (uri.find(loc.rootLoc) == 0 && loc.rootLoc.size() > longestMatch) {
			bestMatch = loc;
			longestMatch = loc.rootLoc.size();
			found = true;
		}
	}
	return found;
}

std::string resolveFilePathFromUri(const std::string& uri, serverLevel& serv) {
	locationLevel loc;
	if (!matchRootLocation(uri, serv, loc)) {
		if (!serv.rootServ.empty()) {
			std::string root = serv.rootServ;
			if (root[root.size() - 1] == '/')
				root = root.substr(0, root.size() - 1);
			if (uri[0] == '/')
				return root + uri;
			else
				return root + "/" + uri;
		} else {
			std::cout << "No root found for URI: " << uri << std::endl;
			if (uri[0] == '/')
				return uri;
			else
				return "/" + uri;
		}
	}
	std::string root = loc.rootLoc;
	std::string locationPath = loc.locName;
	std::string relativeUri = uri.substr(locationPath.size());
	if (!root.empty() && root[root.size() - 1] == '/')
		root = root.substr(0, root.size() - 1);
	if (!relativeUri.empty() && relativeUri[0] == '/')
		relativeUri = relativeUri.substr(1);
	return root + "/" + relativeUri;
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
            "  <p><a href=\"/\">Return to homepage</a></p>\n"
            "  <div class=\"server-info\">\n"
            "    <p>WebServ/1.0</p>\n"
            "  </div>\n"
            "</div>\n"
            "</body>\n"
            "</html>";
}

std::string findErrorPage(int statusCode, Server& server, const std::string& dir) {
    std::map<std::vector<int>, std::string> errorPages = server.getConfigClass().getConfig().errPages;
    std::map<std::vector<int>, std::string>::iterator it = errorPages.begin();
    bool foundCustomPage = false;
	std::string uri;
    while (it != errorPages.end() && !foundCustomPage) {
        // Check if this status code is in the vector of codes
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
    if (foundCustomPage) {// Use custom error page if defined
		if (uri.find(server.getConfigClass().getConfig().rootServ) == std::string::npos)
			filePath = server.getConfigClass().getConfig().rootServ + uri;
		else
			filePath = uri;
	}
    else {// Otherwise use default error page
        filePath = dir + "/" + tostring(statusCode) + ".html";
	}
    return filePath;
}

void resolveErrorResponse(int statusCode, Server& server, std::string& statusText, std::string& body) {
    std::string dir = "errorPages";
    // Look for custom error page for this status code
    std::string filePath = findErrorPage(statusCode, server, dir);
    // Make sure error pages directory exists
    struct stat st;
    if (stat(dir.c_str(), &st) != 0) {
    	mkdir(dir.c_str(), 0755);
	}
    // Try to read existing error page file
    std::ifstream file(filePath.c_str());
    if (file) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        body = buffer.str();
        file.close();
    } else {
        // Generate a basic error page with no external resources
        generateErrorPage(body, statusCode, statusText);
        // Save the generated error page for future use
        std::ofstream out(filePath.c_str());
        if (out) {
            out << body;
            out.close();
        }
    }
}


