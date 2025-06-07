#include "../include/Response.hpp"

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

std::string findErrorPage(int statusCode, const std::string& dir, Request& req) {
    std::map<std::vector<int>, std::string> errorPages = req.getConf().errPages;
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
		if (uri.find(req.getConf().rootServ) == std::string::npos)
			filePath = req.getConf().rootServ + uri;
		else
			filePath = uri;
	}
    else {// Otherwise use default error page
        filePath = dir + "/" + tostring(statusCode) + ".html";
	}
    return filePath;
}

void resolveErrorResponse(int statusCode, std::string& statusText, std::string& body, Request& req) {
    std::string dir = "errorPages";
    // Look for custom error page for this status code
    std::string filePath = findErrorPage(statusCode, dir, req);
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


