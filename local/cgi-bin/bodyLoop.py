#!/usr/bin/env python3
import time
import sys

def main():
    sys.stdout.reconfigure(line_buffering=True)
    
    counter = 0
    while True:
        print(f"Output {counter}")
        sys.stdout.flush()
        counter += 1
        time.sleep(1)

if __name__ == "__main__":
    main()