#!/usr/bin/env python3

import subprocess
import sys
import time
import os

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
    
    try:
        for script in scripts:
            print(f"__________ Running {script}... __________")
            result = subprocess.run([sys.executable, script], 
                                  capture_output=True, 
                                  text=True, 
                                  check=True,
                                  cwd=os.path.abspath("."))
            if result.stdout:
                print(f"{result.stdout}")
            print(f"✓ {script} completed successfully\n")
            time.sleep(2)
            
    except subprocess.CalledProcessError as err:
        print(f"✗ Error running {script}: {err}")
        print(f"Error output: {err.stderr}")
    except FileNotFoundError:
        print(f"✗ Script not found: {script}")
    finally:
        print("=============== ALL DONE ===============")

if __name__ == "__main__":
    main()