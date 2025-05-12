#!/usr/bin/python3
import os
import sys
import urllib.parse

# Output headers first
print("Content-Type: text/html\r\n\r\n")

# Start HTML
print("<!DOCTYPE html>")
print("<html>")
print("<head>")
print("<title>Hello from Python CGI</title>")
print("<style>body { font-family: Arial, sans-serif; margin: 20px; }</style>")
print("<link rel=\"icon\" href=\"/images/favicon.ico\" type=\"image/x-icon\">")
print("</head>")
print("<body>")
print("<h1>Hello from Python CGI!</h1>")
print("<div><a href=\"../cgi.html\">Back</a></div>")


# Get request method
method = os.environ.get("REQUEST_METHOD", "Unknown")
print(f"<p><strong>Request Method:</strong> {method}</p>")

# Handle GET params
query_string = os.environ.get("QUERY_STRING", "")
print(f"<p><strong>Query String:</strong> {query_string}</p>")

# Parse query string
if query_string:
    # Parse query parameters
    params = urllib.parse.parse_qs(query_string)
    print("<h2>Query Parameters:</h2>")
    print("<ul>")
    for key, values in params.items():
        for value in values:
            print(f"<li><strong>{key}:</strong> {value}</li>")
    print("</ul>")
    
    # Special greeting if name parameter exists
    if 'name' in params:
        name = params['name'][0]
        print(f"<h3>Hello, {name}!</h3>")

# Handle POST data
if method == "POST":
    content_type = os.environ.get("CONTENT_TYPE", "")
    content_length = int(os.environ.get("CONTENT_LENGTH", 0))
    
    print(f"<p><strong>Content Type:</strong> {content_type}</p>")
    print(f"<p><strong>Content Length:</strong> {content_length}</p>")
    
    if content_length > 0:
        # Read POST data
        post_data = sys.stdin.read(content_length)
        print("<h2>POST Data (raw):</h2>")
        print(f"<pre>{post_data}</pre>")
        
        # Parse application/x-www-form-urlencoded data
        if content_type.startswith("application/x-www-form-urlencoded"):
            try:
                form_data = urllib.parse.parse_qs(post_data)
                if form_data:
                    print("<h2>Form Parameters:</h2>")
                    print("<ul>")
                    for key, values in form_data.items():
                        for value in values:
                            print(f"<li><strong>{key}:</strong> {value}</li>")
                    print("</ul>")
                    
                    # Special greeting if message parameter exists
                    if b'message' in form_data:
                        message = form_data[b'message'][0].decode('utf-8')
                        print(f"<h3>Your message: {message}</h3>")
            except Exception as e:
                print(f"<p>Error parsing form data: {str(e)}</p>")

# End HTML
print("</body>")
print("</html>")