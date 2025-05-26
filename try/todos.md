**TODOs**

in config file:
	- server_name (host header) smth.com #used for virtual hosting, deciding which config to use based on the host header

## 5. Configuration File Implementation

### 5.1 Configuration File Format

Your server should support a configuration file similar to NGINX's format. Example:

```
server {
    listen 8080;
    server_name example.com;
    error_page 404 /404.html;
    client_max_body_size 10M;
    
    location / {
        root /var/www/html;
        index index.html;
        methods GET POST;
        autoindex on;
    }
    
    location /uploads {
        root /var/www/uploads;
        methods POST DELETE;
        upload_store /var/www/uploads;
    }
    
    location ~ \.php$ {
        root /var/www/php;
        cgi_pass /usr/bin/php-cgi;
        methods GET POST;
    }
}

server {
    listen 8081;
    server_name api.example.com;
    # ...
}
```

### 5.2 Configuration Parameters

Your server should support these configuration parameters:

1. **Server Level:**
   - **listen**: Port number
   - **server_name**: Server identification
   - **error_page**: Custom error pages
   - **client_max_body_size**: Maximum request body size

2. **Location Level:**
   - **root**: Document root directory
   - **index**: Default file for directory requests
   - **methods**: Allowed HTTP methods
   - **autoindex**: Directory listing (on/off)
   - **redirect**: HTTP redirection
   - **cgi_pass**: CGI processor path
   - **upload_store**: Upload destination

### 5.3 Parsing Configuration

Implement a robust parser for your configuration file:

1. Read the file line by line
2. Tokenize and validate syntax
3. Build a hierarchical representation of the configuration
4. Apply default values for unspecified parameters
5. Detect and report configuration errors


**🧾 TL;DR — Minimum Required in a Config File**
 *🔧 At the top level (inside server {} block):*
	Directive				Required?		 Why it matters
	listen					✅ YES			Tells the server which port/IP to listen on
	server_name				✅ YES			Used to match incoming requests (vhosts)
	root					✅ YES			Tells where to look for static files
	index					✅ YES			Specifies default file in directories
	location				✅ YES			To define behavior for URL paths
	error_page				⚠️ Optional		 Useful for custom error responses
	client_max_body_size	⚠️ Optional		 Used to limit POST/PUT request size

 *🗂 Inside a location {} block:*
	Directive				Required?		 Why it matters
	root					✅ YES			Can override or inherit from server root
	index					✅ YES			Same as above, but per location
	allow_methods			⚠️ Recommended	 Defines which HTTP methods are allowed
	cgi_pass				⚠️ Optional		 If handling PHP/CGI scripts
	autoindex				⚠️ Optional		 Enables directory listing if no index file

**Missing thing	Resulting problem**
	listen									Server won’t bind to any port
	server_name								Virtual hosting may break / default fallback
	root									Server doesn’t know where to serve files from
	index									Requests to / may return 403 or 404
	location block							All URL requests fall back to root config