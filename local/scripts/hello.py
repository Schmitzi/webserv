#!/usr/bin/python3
import os
import urllib.parse

# Output headers first
print("Content-Type: text/html\r\n\r\n")

# Start HTML
print("<!DOCTYPE html>")
print("<html>")
print("<head><title>Hello from Python CGI</title></head>")
print("<body>")
print("<h1>Hello from CGI!</h1>")

# Get request method
method = os.environ.get("REQUEST_METHOD", "Unknown")
print(f"<p>Method: {method}</p>")

# Get and parse query string
query_string = os.environ.get("QUERY_STRING", "")
print(f"<p>Query String: {query_string}</p>")

# Parse query string without using the cgi module
if query_string:
    # Parse query parameters
    params = urllib.parse.parse_qs(query_string)
    print("<h2>Query Parameters:</h2>")
    print("<ul>")
    for key, values in params.items():
        for value in values:
            print(f"<li>{key}: {value}</li>")
    print("</ul>")
    
    # Special greeting if name parameter exists
    if 'name' in params:
        name = params['name'][0]
        print(f"<h3>Hello, {name}!</h3>")

# End HTML
print("</body>")
print("</html>")