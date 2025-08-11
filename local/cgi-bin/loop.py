#!/usr/bin/python3

import sys
import time

# Print CGI headers first (required for CGI)
print("Content-Type: text/html\r\n\r\n")
print("<html><body>")
print("<h1>Starting infinite loop test...</h1>")
print("<p>This script should be killed by the server after 14 seconds.</p>")
sys.stdout.flush()  # Ensure headers are sent

# Now start the infinite loop
counter = 0
while True:
    time.sleep(1)  # Sleep for 1 second each iteration
    counter += 1
    # Optional: print something every few seconds (but this might not be visible)
    if counter % 5 == 0:
        print(f"<p>Still running... {counter} seconds</p>")
        sys.stdout.flush()
