#!/usr/bin/env python3

import os
import subprocess
import sys
import time
from colorama import Fore, Style

# === GLOBAL COUNTERS ===
PASS_COUNT = 0
TOTAL_COUNT = 0

# === COLORS ===
def success(msg): print(f"{Fore.GREEN}✅ {msg}{Style.RESET_ALL}")
def fail(msg): print(f"{Fore.RED}❌ {msg}{Style.RESET_ALL}")
def header(msg): print(f"{Fore.CYAN}\n===> _____{msg}_____<===\n{Style.RESET_ALL}")

# === UTILITIES ===
def run_test(description, func):
    global PASS_COUNT, TOTAL_COUNT
    TOTAL_COUNT += 1
    header(f"{TOTAL_COUNT}. {description}")
    try:
        func()
        success("Success")
        PASS_COUNT += 1
    except Exception as e:
        fail("Failed")
        print(e)

def run_script(path):
    result = subprocess.run([sys.executable, path],
                            capture_output=True,
                            text=True,
                            check=True,
                            cwd=os.path.abspath("."))
    if result.stdout:
        print(result.stdout)

def main():
    scripts = [
        "tests/webAnother.py",#TODO: chunked test doesnt work anymore :(
        "tests/webCheck.py",
        "tests/webCheckFull.py",#TODO: large Post body-> 500 internal server error??
        "tests/webChunk.py",
        "tests/webTest.py",#TODO: Error Handeling-> get /forbidden==404?, Dir Listing-> autoindex is not off, HTTP redirs-> looks wrong, CGI func-> 500 server error??, testing multiple ports
        "tests/webTestCgiFileArg.py",
        "tests/webUriEncoding.py"
    ]

    for script in scripts:
        run_test(f"Running {script}", lambda: run_script(script))
        time.sleep(1)

    print("\n=====================================")
    print(f"✅ {Fore.GREEN}{PASS_COUNT}{Style.RESET_ALL} / {TOTAL_COUNT} scripts ran successfully")
    print("=====================================")

if __name__ == "__main__":
    main()
