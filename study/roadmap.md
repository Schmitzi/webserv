# Webserv Development Roadmap

## 1. HTTP Request Parsing
- Create an HTTP Request class that can:
  - Parse request line (method, URI, version)
  - Parse headers (name-value pairs)
  - Handle request body for POST methods
  - Support chunked transfer encoding

## 2. Implement Configuration System
- Implement proper configuration file parsing (similar to NGINX syntax)
- Support for multiple server blocks with different hosts/ports
- Route configuration with accepted methods, redirects, etc.
- Error page settings
- Client body size limits

## 3. HTTP Response Generation
- Create an HTTP Response class that:
  - Sets appropriate status codes
  - Generates correct headers
  - Includes proper response body
  - Implements all required HTTP methods (GET, POST, DELETE)

## 4. Static File Serving
- Add functionality to serve files from the filesystem
- Support for directory listing (when enabled)
- Default file serving for directory requests
- Proper MIME type detection

## 5. CGI Support
- Implement CGI execution with proper environment setup
- Handle input/output from CGI processes
- Support for file uploads through POST

## 6. Connection Management
- Implement non-blocking I/O with poll() or equivalent
- Handle multiple concurrent connections
- Properly manage client disconnections
- Implement proper timeout handling

## 7. Error Handling
- Generate appropriate error pages
- Implement robust error recovery
- Add detailed logging

## 8. Testing & Optimization
- Thorough testing against browsers
- Stress testing
- Performance optimization

## Specific Code Improvements

1. In your `Client` class, currently the `recieveData()` method just echoes data back. Enhance this to:
   - Parse HTTP requests
   - Route to appropriate handlers

2. Your `Config` class needs a proper implementation for parsing config files

3. Modify the `Webserv::run()` method to:
   - Use poll() instead of a simple blocking loop
   - Handle multiple clients simultaneously

4. Add proper HTTP protocol elements:
   - Request parsing
   - Response generation
   - Header management

5. Implement the proper file and directory handling logic
