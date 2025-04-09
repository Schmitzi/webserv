#!/usr/bin/env python3
import os
import sys
import cgi

# Ensure script is executable
# chmod +x /local/scripts/hello.py

# Explicit error handling
try:
    # Print headers
    print("Content-Type: text/html")
    print()  # Extra newline to separate headers from body

    # Parse query string
    query_string = os.environ.get('QUERY_STRING', '')
    form = cgi.parse_qs(query_string)

    # Access environment variables and query parameters
    print("<html><body>")
    print("<h1>Hello from CGI!</h1>")
    print(f"<p>Method: {os.environ.get('REQUEST_METHOD')}</p>")
    print(f"<p>Query String: {query_string}</p>")

    # Print query parameters
    if 'name' in form:
        print(f"<p>Name: {form['name'][0]}</p>")

    print("</body></html>")

except Exception as e:
    # Print error to stderr
    print("Status: 500 Internal Server Error")
    print("Content-Type: text/plain")
    print()
    print(f"CGI Script Error: {e}")
    sys.stderr.write(f"CGI Script Error: {e}\n")
    sys.exit(1)