Based on the Webserv project PDF, here's a comprehensive checklist of requirements:

## Core Requirements

### General Rules
- [ ] Code must be C++98 compliant (-std=c++98 flag)
- [ ] Must compile with flags: -Wall -Wextra -Werror
- [ ] Makefile must include: $(NAME), all, clean, fclean, re
- [ ] No external libraries or Boost allowed
- [ ] Program must never crash or exit unexpectedly
- [ ] No memory leaks

### Network Architecture
- [ ] Must use non-blocking sockets
- [ ] Use only 1 poll() (or equivalent like select, epoll, kqueue) for ALL I/O operations
- [ ] poll() must monitor both reading and writing
- [ ] Never perform read/write without going through poll()
- [ ] Cannot check errno after read/write operations
- [ ] Server must accept an optional configuration file argument

### HTTP Implementation
- [ ] Support GET, POST, and DELETE methods
- [ ] Accurate HTTP response status codes
- [ ] Serve fully static websites
- [ ] Handle file uploads
- [ ] Default error pages if none provided
- [ ] Compatible with standard web browsers
- [ ] Listen on multiple ports (configured)
- [ ] Handle multiple clients simultaneously
- [ ] Clients should never hang indefinitely

### Features to Implement
- [ ] Parse and validate HTTP requests
- [ ] Serve static files with correct MIME types
- [ ] Handle multipart/form-data uploads
- [ ] Directory listing (if enabled in config)
- [ ] HTTP redirects
- [ ] Client body size limits
- [ ] CGI execution (at least PHP or Python)
- [ ] Proper CGI environment variables
- [ ] Handle chunked requests for CGI

### Configuration File Features
- [ ] Configure port and host for each server
- [ ] Set server_names
- [ ] Default server for host:port
- [ ] Custom error pages
- [ ] Client body size limits
- [ ] Route configuration with:
  - [ ] Accepted HTTP methods per route
  - [ ] HTTP redirects
  - [ ] Root directory/file location
  - [ ] Directory listing enable/disable
  - [ ] Default file for directories
  - [ ] CGI execution based on file extension
  - [ ] Upload location configuration

### CGI Requirements
- [ ] Execute CGI with file as first argument
- [ ] Run CGI in correct directory
- [ ] Pass full path as PATH_INFO
- [ ] Handle chunked requests (unchunk before CGI)
- [ ] Handle CGI output (chunked responses)
- [ ] Fork only for CGI execution
- [ ] Support at least one CGI interpreter (PHP/Python)

### Error Handling
- [ ] Proper error responses with status codes
- [ ] Default error pages
- [ ] Handle malformed requests
- [ ] Handle file not found
- [ ] Handle permission errors
- [ ] Handle CGI errors

### Testing Requirements
- [ ] Must remain operational under stress
- [ ] Compare behavior with NGINX when in doubt
- [ ] Provide test configuration files
- [ ] Test with multiple clients
- [ ] Test with different browsers

### MacOS Specific (if applicable)
- [ ] Use fcntl() only with F_SETFL, O_NONBLOCK, FD_CLOEXEC flags
- [ ] File descriptors must be non-blocking

### Bonus Features (only if mandatory part is perfect)
- [ ] Cookie support
- [ ] Session management
- [ ] Multiple CGI interpreters

Make sure to also:
- Test with telnet and actual browsers
- Read relevant RFCs for HTTP/1.1
- Compare behavior with NGINX for edge cases
- Write stress tests in various languages
- Ensure the server never hangs or crashes