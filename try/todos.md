**TODOs**

-> change hardcoded values of request to actual values of getConfigValue()
-> maybe implement different split and itoa?
-> make default error page(s)

in config file:
	- listen/port 8080; #can have multiple
	- server_name (host header) smth.com #used for virtual hosting, deciding which config to use based on the host header
	- root (document root) "path to smth"; #directory where the files for this server are stored
	- index smth.html; #default file to return if a directory is requested
	- error_page 404 /404.html; #custom error pages
	- location blocks:
		- location / {
			try_files $smth $smth/ =404;
		}
		- location /images/ {
			root "path to smth";
		}
	- cgi_pass "path to smth";
	- cgi_extension .py(?);
	- limit_except GET POST DELETE {
		deny all; (?) #specify which methods are allowed
	}
	- client_max_body_size 1m;
	- autoindex on; #show directory listing if no index file is present?

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