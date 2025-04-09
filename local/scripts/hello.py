#!/usr/bin/env python3
import os
import sys

# Print headers (optional)
print("Content-Type: text/html")
print()  # Extra newline to separate headers from body

# Access environment variables
print("<html><body>")
print("<h1>Hello from CGI!</h1>")
print(f"<p>Method: {os.environ.get('REQUEST_METHOD')}</p>")
print(f"<p>Query String: {os.environ.get('QUERY_STRING')}</p>")

# Read input if any
input_data = sys.stdin.read()
if input_data:
    print(f"<p>Input Data: {input_data}</p>")

print("</body></html>")