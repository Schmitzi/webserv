#!/usr/bin/env python3

import os
import subprocess
import sys
import time
from colorama import Fore, Style

# === COLORS ===
def success(msg): print(f"{Fore.GREEN}✅ {msg}{Style.RESET_ALL}")
def fail(msg): print(f"{Fore.RED}❌ {msg}{Style.RESET_ALL}")
def header(msg): print(f"{Fore.CYAN}\n===> _____{msg}_____<===\n{Style.RESET_ALL}")

PASS_COUNT = 0      # total passed test cases
TOTAL_COUNT = 0     # total test cases run
TOTAL_SCRIPTS = 0   # total scripts run

def run_test(description, func):
    global PASS_COUNT, TOTAL_COUNT, TOTAL_SCRIPTS
    TOTAL_SCRIPTS += 1
    header(f"{TOTAL_SCRIPTS}. {description}")
    try:
        output = func()
        import re
        m = re.search(r"SUMMARY: PASS (\d+) / (\d+)", output)
        if m:
            passed = int(m.group(1))
            total = int(m.group(2))
            success(f"Success: {passed} / {total} test cases passed")
            PASS_COUNT += passed
            TOTAL_COUNT += total
        else:
            success("Success (summary missing, counting as 1 pass)")
            PASS_COUNT += 1
            TOTAL_COUNT += 1
        print(output)
    except Exception as e:
        fail("Failed")
        print(e)

def run_script(path):
    result = subprocess.run([sys.executable, path],
                            capture_output=True,
                            text=True,
                            check=True,
                            cwd=os.path.abspath("."))
    return result.stdout

def main():
    scripts = [
        "tests/webAnother.py",
        "tests/webCheck.py",
        "tests/webCheckFull.py",
        "tests/webChunk.py",
        "tests/webTest.py",
        "tests/webTestCgiFileArg.py",
        "tests/webUriEncoding.py"
    ]

    for script in scripts:
        run_test(f"Running {script}", lambda: run_script(script))
        time.sleep(1)

    print("\n=====================================")
    print(f"✅ {Fore.GREEN}{PASS_COUNT}{Style.RESET_ALL} / {TOTAL_COUNT} total test cases passed")
    print("=====================================")

if __name__ == "__main__":
    main()