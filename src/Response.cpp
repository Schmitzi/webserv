#include "../include/Response.hpp"
#include "../include/Config.hpp"

const locationLevel* matchLocation(const std::string& uri, const serverLevel& serv) {
	const locationLevel* bestMatch = NULL;
	size_t longestMatch = 0;

	std::map<std::string, locationLevel>::const_iterator it = serv.locations.begin();
	for (; it != serv.locations.end(); ++it) {
		const locationLevel& loc = it->second;
		if (uri.find(loc.rootLoc) == 0 && loc.rootLoc.size() > longestMatch) {
			bestMatch = &loc;
			longestMatch = loc.rootLoc.size();
		}
	}
	return bestMatch;
}

std::string resolveFilePathFromUri(const std::string& uri, const serverLevel& serv) {
	const locationLevel* loc = matchLocation(uri, serv);
	if (!loc) {
		std::cout << "Location not found for URI: " << uri << std::endl;
		if (uri[0] == '/')
			return uri;
		else
			return "/" + uri;
	}
	std::string root = loc->rootLoc;
	std::string locationPath = loc->locName;
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

// Fix for resolveErrorResponse function in Response.cpp
// Corrected resolveErrorResponse function in Response.cpp
void resolveErrorResponse(int statusCode, Webserv& webserv, std::string& statusText, std::string& body) {
    std::string dir = "errorPages";
    std::map<std::vector<int>, std::string> errorPages = webserv.getConfig().getConfig().errPages;
    std::map<std::vector<int>, std::string>::iterator it = errorPages.begin();
    
    // Look for custom error page for this status code
    bool foundCustomPage = false;
    while (it != errorPages.end()) {
        // Check if this status code is in the vector of codes
        for (size_t i = 0; i < it->first.size(); i++) {
            if (it->first[i] == statusCode) {
                foundCustomPage = true;
                break;
            }
        }
        if (foundCustomPage) {
            break;
        }
        ++it;
    }
    
    std::string filePath;
    if (foundCustomPage) {
        // Use custom error page if defined
        std::string uri = it->second;
        filePath = webserv.getConfig().getConfig().rootServ + uri;
    } else {
        // Otherwise use default error page
        filePath = dir + "/" + tostring(statusCode) + ".html";
    }
    
    // Make sure error pages directory exists
    struct stat st;
    if (stat(dir.c_str(), &st) != 0)
        mkdir(dir.c_str(), 0755);
    
    // Try to read existing error page file
    std::ifstream file(filePath.c_str());
    if (file) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        body = buffer.str();
        file.close();
    } else {
        // Generate a basic error page with no external resources
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
        
        // Save the generated error page for future use
        std::ofstream out(filePath.c_str());
        if (out) {
            out << body;
            out.close();
        }
    }
}


