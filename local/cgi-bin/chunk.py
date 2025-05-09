#!/usr/bin/env python3
"""
This is a test CGI script that demonstrates chunked encoding.
Put this in your cgi-bin directory and make it executable:
chmod +x chunked_test.py
"""

import sys
import time

# Output headers
print("Content-Type: text/html")
print("Transfer-Encoding: chunked")
print()  # Empty line to indicate end of headers

sys.stdout.flush()  # Force headers to be sent immediately

# Now we'll send chunks of data with deliberate delays
# to simulate slow generation of content

# Chunk 1
html_start = """<!DOCTYPE html>
<html>
<head>
    <title>Chunked Transfer Test</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .chunk { padding: 10px; margin: 10px 0; border: 1px solid #ccc; }
        .timestamp { color: #999; font-size: 0.8em; }
    </style>
</head>
<body>
    <h1>Chunked Transfer Encoding Test</h1>
    <p>This page is being delivered using HTTP chunked transfer encoding.</p>
    <div class="chunk">
        <h3>Chunk 1</h3>
        <p>This is the first chunk of data.</p>
        <p class="timestamp">Sent at: """

print(html_start)
print(time.strftime("%H:%M:%S"))
print("</p>\n    </div>")
sys.stdout.flush()  # Flush to ensure chunk is sent

# Simulate a delay in generating content
time.sleep(1)

# Chunk 2
print('    <div class="chunk">')
print('        <h3>Chunk 2</h3>')
print('        <p>This is the second chunk of data, sent after a delay.</p>')
print('        <p class="timestamp">Sent at: ' + time.strftime("%H:%M:%S") + '</p>')
print('    </div>')
sys.stdout.flush()

# Another delay
time.sleep(1)

# Chunk 3
print('    <div class="chunk">')
print('        <h3>Chunk 3</h3>')
print('        <p>This is the third and final chunk of data.</p>')
print('        <p>The server can send data as it becomes available, rather than waiting for the entire response to be ready.</p>')
print('        <p class="timestamp">Sent at: ' + time.strftime("%H:%M:%S") + '</p>')
print('    </div>')
print('    <p>All chunks have been delivered successfully.</p>')
print('</body>')
print('</html>')
sys.stdout.flush()