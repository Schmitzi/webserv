Based on the Webserv project PDF, here's a comprehensive checklist of requirements:

## Core Requirements

### General Rules
- [ ] Code must be C++98 compliant (-std=c++98 flag) &#x2611;
- [ ] Must compile with flags: -Wall -Wextra -Werror &#x2611;
- [ ] Makefile must include: $(NAME), all, clean, fclean, re &#x2611;
- [ ] No external libraries or Boost allowed &#x2611;
- [ ] Program must never crash or exit unexpectedly &#x2611;
- [ ] No memory leaks &#x2611;

### Network Architecture
- [ ] Must use non-blocking sockets &#x2611;
- [ ] Use only 1 poll() (or equivalent like select, epoll, kqueue) for ALL I/O operations &#x2611;
- [ ] poll() must monitor both reading and writing &#x2611;
- [ ] Never perform read/write without going through poll() &#x2611;
- [ ] Cannot check errno after read/write operations &#x2611;
- [ ] Server must accept an optional configuration file argument &#x2611;

### HTTP Implementation
- [ ] Support GET, POST, and DELETE methods &#x2611;
- [ ] Accurate HTTP response status codes &#x2611;
- [ ] Serve fully static websites &#x2611;
- [ ] Handle file uploads &#x2611;
- [ ] Default error pages if none provided &#x2611;
- [ ] Compatible with standard web browsers &#x2611;
- [ ] Listen on multiple ports (configured) &#x2611;
- [ ] Handle multiple clients simultaneously &#x2611;
- [ ] Clients should never hang indefinitely &#x2611;

### Features to Implement
- [ ] Parse and validate HTTP requests &#x2611;
- [ ] Serve static files with correct MIME types &#x2611;
- [ ] Handle multipart/form-data uploads &#x2611;
- [ ] Directory listing (if enabled in config) &#x2611;
- [ ] HTTP redirects &#x2611;
- [ ] Client body size limits &#x2611;
- [ ] CGI execution (at least PHP or Python) &#x2611;
- [ ] Proper CGI environment variables &#x2611;
- [ ] Handle chunked requests for CGI &#x2611;

### Configuration File Features
- [ ] Configure port and host for each server &#x2611;
- [ ] Set server_names &#x2611;
- [ ] Default server for host:port &#x2611;
- [ ] Custom error pages &#x2611;
- [ ] Client body size limits &#x2611;
- [ ] Route configuration with:
  - [ ] Accepted HTTP methods per route
  - [ ] HTTP redirects &#x2611;
  - [ ] Root directory/file location &#x2611;
  - [ ] Directory listing enable/disable &#x2611;
  - [ ] Default file for directories
  - [ ] CGI execution based on file extension &#x2611;
  - [ ] Upload location configuration &#x2611; 

### CGI Requirements
- [ ] Execute CGI with file as first argument 
- [ ] Run CGI in correct directory &#x2611;
- [ ] Pass full path as PATH_INFO
- [ ] Handle chunked requests (unchunk before CGI) &#x2611;
- [ ] Handle CGI output (chunked responses) &#x2611;
- [ ] Fork only for CGI execution &#x2611;
- [ ] Support at least one CGI interpreter (PHP/Python) &#x2611;

### Error Handling
- [ ] Proper error responses with status codes &#x2611;
- [ ] Default error pages &#x2611;
- [ ] Handle malformed requests &#x2611;
- [ ] Handle file not found &#x2611;
- [ ] Handle permission errors &#x2611;
- [ ] Handle CGI errors &#x2611;

### Testing Requirements
- [ ] Must remain operational under stress
- [ ] Compare behavior with NGINX when in doubt
- [ ] Provide test configuration files
- [ ] Test with multiple clients
- [ ] Test with different browsers &#x2611;

### Bonus Features (only if mandatory part is perfect)
- [ ] Cookie support
- [ ] Session management
- [ ] Multiple CGI interpreters &#x2611;

Make sure to also:
- Test with telnet and actual browsers
- Read relevant RFCs for HTTP/1.1
- Compare behavior with NGINX for edge cases
- Write stress tests in various languages
- Ensure the server never hangs or crashes