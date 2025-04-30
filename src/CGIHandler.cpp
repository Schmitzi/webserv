#include "../include/CGIHandler.hpp"
#include "../include/Client.hpp"

CGIHandler::CGIHandler() : _args(NULL) {
    // Initialize arrays
    _input[0] = -1;
    _input[1] = -1;
    _output[0] = -1;
    _output[1] = -1;
}

CGIHandler::~CGIHandler() {
    cleanupResources();
}

void    CGIHandler::setClient(Client &client) {
    _client = &client;
}

int CGIHandler::executeCGI(Client &client, Request &req, std::string const &scriptPath) {
    cleanupResources();
    _path = scriptPath;
    
    if (doChecks(client) == 1) {
        return 1;
    }

    prepareEnv(req);

    pid_t pid = fork();
    
    if (pid == 0) {  // Child process
        close(_input[1]);
        close(_output[0]);
        
        if (dup2(_input[0], STDIN_FILENO) < 0 || 
            dup2(_output[1], STDOUT_FILENO) < 0) {
            std::cerr << "dup2 failed\n";
            exit(1);
        }
        
        close(_input[0]);
        close(_output[1]);
        
        execve(_args[0], _args, &_env[0]); //"/usr/bin/env"
        
        std::cerr << "execve failed: " << strerror(errno) << "\n";
        exit(1);
    } 
    else if (pid > 0) {  // Parent process
        close(_input[0]);
        close(_output[1]);

        if (!req.getBody().empty()) {//TODO: check for requestLimit?
            write(_input[1], req.getBody().c_str(), req.getBody().length());
        }
        close(_input[1]);
        _input[1] = -1;

        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            std::cerr << "CGI Script exit status: " << exit_status << std::endl;
            
            if (exit_status == 0) {
                int result = processScriptOutput(client);
                cleanupResources();
                return result;
            } else {
                client.sendErrorResponse(500);//, " - CGI Script Execution Failed");
                cleanupResources();
                return 1;
            }
        } else {
            client.sendErrorResponse(500);//, " - CGI Script Terminated Abnormally");
            cleanupResources();
            return 1;
        }
    }
    
    client.sendErrorResponse(500);//, " - Fork failed");
    cleanupResources();
    return 1;
}

int CGIHandler::processScriptOutput(Client &client) {
    std::string output;
    char buffer[4096];
    int totalBytesRead = 0;
    
    int flags = fcntl(_output[0], F_GETFL, 0);
    fcntl(_output[0], F_SETFL, flags | O_NONBLOCK);
    
    fd_set readfds;
    
    FD_ZERO(&readfds);
    FD_SET(_output[0], &readfds);
    
    int selectResult = select(_output[0] + 1, &readfds, NULL, NULL, 0); //
    
    if (selectResult > 0) {
        ssize_t bytesRead;
        while ((bytesRead = read(_output[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;
            totalBytesRead += bytesRead;
        }
    }
    
    // Close output pipe
    close(_output[0]);
    _output[0] = -1;

    // Debug print
    std::cout << "CGI Output:\n" << output << std::endl;
    std::cout << "Total bytes read: " << totalBytesRead << std::endl;

    // If no output, send a default response
    if (output.empty()) {
        std::string defaultResponse = "HTTP/1.1 200 OK\r\n";
        defaultResponse += "Content-Type: text/plain\r\n";
        defaultResponse += "Content-Length: 22\r\n\r\n";
        defaultResponse += "No output from script\n";
        
        send(client.getFd(), defaultResponse.c_str(), defaultResponse.length(), 0);
        return 0;
    }

    // Parse CGI output
    Request temp = createTempHeader(output);
    
	_client->sendResponse(temp, "keep-alive", temp.getBody());
	// _client->sendResponse(temp, 200, "keep-alive");//TODO: check what it returns?
    return 0;
}

bool CGIHandler::isCGIScript(const std::string& path) {
    size_t dotPos = path.find_last_of('.');
    
    if (dotPos != std::string::npos) {
        std::string ext = path.substr(dotPos + 1);
        
        static const char* whiteList[] = {"py", "php", "cgi", "pl", NULL};
        
        for (int i = 0; whiteList[i] != NULL; ++i) {
            if (ext == whiteList[i]) {
                return true;
            }
        }
    }
    
    return false;
}

void CGIHandler::prepareEnv(Request &req) {
    _env.clear();

    std::vector<std::string> tempEnv;
    tempEnv.push_back("REQUEST_METHOD=" + req.getMethod());
    tempEnv.push_back("QUERY_STRING=" + req.getQuery());
    tempEnv.push_back("CONTENT_TYPE=" + req.getContentType());
    tempEnv.push_back("CONTENT_LENGTH=" + tostring(req.getBody().length()));
    tempEnv.push_back("SCRIPT_NAME=" + req.getPath());
    tempEnv.push_back("SERVER_SOFTWARE=WebServ/1.0");

    for (std::vector<std::string>::iterator it = tempEnv.begin(); it != tempEnv.end(); ++it) {
        char* envStr = strdup(it->c_str());
        if (envStr) {
            _env.push_back(envStr);
        }
    }
    _env.push_back(NULL);

    if (_pythonPath == "/usr/bin/env") {
        _args = new char*[4];
        _args[0] = strdup(_pythonPath.c_str());
        _args[1] = strdup("python3");
        _args[2] = strdup(_path.c_str());
        _args[3] = NULL;
    } else {
        _args = new char*[3];
        _args[0] = strdup(_pythonPath.c_str());
        _args[1] = strdup(_path.c_str());
        _args[2] = NULL;
    }
}

void CGIHandler::cleanupResources() {
    for (size_t i = 0; i < _env.size(); i++) {
        if (_env[i]) {
            free(_env[i]);
        }
    }
    _env.clear();
    
    if (_args) {
        for (int i = 0; _args[i]; i++) {
            free(_args[i]);
        }
        delete[] _args;
        _args = NULL;
    }
    
    if (_input[0] >= 0) 
        close(_input[0]);
    if (_input[1] >= 0)
        close(_input[1]);
    if (_output[0] >= 0) 
        close(_output[0]); 
    if (_output[1] >= 0) 
        close(_output[1]);
}

void CGIHandler::setPythonPath(char **envp) {  // Please dont judge me 
    std::string path;
    
    for (int i = 0; envp[i]; i++) {
        std::string env(envp[i]);
        if (env.substr(0, 5) == "PATH=") {
            path = env.substr(5);
            break;
        }
    }
    
    if (path.empty()) {
        std::cout << "Warning: PATH environment variable not found" << std::endl;
        _pythonPath = "/usr/bin/env";
        return;
    }
    
    std::vector<std::string> paths;
    size_t start = 0;
    size_t end = path.find(':');
    
    while (end != std::string::npos) {
        paths.push_back(path.substr(start, end - start));
        start = end + 1;
        end = path.find(':', start);
    }
    
    paths.push_back(path.substr(start));
    
    for (std::vector<std::string>::iterator it = paths.begin(); it != paths.end(); ++it) {
        std::string newPath = *it + "/python3";
        if (access(newPath.c_str(), X_OK) == 0) {
            _pythonPath = newPath;
            std::cout << "Found Python at: " << _pythonPath << std::endl;
            return;
        }
    }
    
    std::cout << "Python not found in PATH, using env instead" << std::endl;
    _pythonPath = "/usr/bin/env";
}

Request    CGIHandler::createTempHeader(std::string output) {
    size_t headerEnd = output.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = output.find("\n\n");
    }

    Request temp;
    temp.setContentType("text/html");
    temp.setBody(output);
    std::string headers = "Content-Type: text/html\r\n";

    if (headerEnd != std::string::npos) {
        headers = output.substr(0, headerEnd);
        temp.setBody(output.substr(headerEnd + (output.find("\r\n\r\n") != std::string::npos ? 4 : 2)));

        size_t typePos = headers.find("Content-Type:");
        if (typePos != std::string::npos) {
            std::string tempType = temp.getContentType();
            size_t lineEnd = headers.find("\n", typePos);
            temp.setContentType(headers.substr(typePos + 13, lineEnd - typePos - 13));
            tempType.erase(0, tempType.find_first_not_of(" \t"));
            tempType.erase(tempType.find_last_not_of(" \t\r\n") + 1);
            temp.setContentType(tempType);
        }
    }
    return temp;
}

int CGIHandler::doChecks(Client client) {
    if (access(_path.c_str(), F_OK) != 0) {
        std::cerr << "Script does not exist: " << _path << std::endl;
        client.sendErrorResponse(404);//, " - CGI Script Not Found");
        return 1;
    }

    if (access(_path.c_str(), X_OK) != 0) {
        std::cerr << "Script is not executable: " << _path << std::endl;
        client.sendErrorResponse(403);//, " - CGI Script Not Executable");
        return 1;
    }

    if (pipe(_input) < 0 || pipe(_output) < 0) {
        std::cerr << "Pipe creation failed" << std::endl;
        client.sendErrorResponse(500);//, " - Pipe creation failed");
        return 1;
    }
    return 0;
}