#!/usr/bin/env python3
import sys
print("Content-Type: text/plain\n")
print("Host: abc.com"\n)
try:
    with open(sys.argv[1], 'r') as file:
        print(file.read())
except Exception as e:
    print("Error:", e)
