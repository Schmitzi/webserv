#!/usr/bin/env python3

import subprocess
import sys
import time

def main():
    scripts = [
        "webCheck.py",
        "webCheckFull.py", 
        "webChunk.py",
        "webTest.py",
        "webTestCgiFileArg.py"
    ]
    
    try:
        for script in scripts:
            print(f"Running {script}...")
            result = subprocess.run([sys.executable, script], 
                                  capture_output=True, 
                                  text=True, 
                                  check=True)
            print(f"✓ {script} completed successfully")
            if result.stdout:
                print(f"Output: {result.stdout}")
            time.sleep(2)
            
    except subprocess.CalledProcessError as err:
        print(f"✗ Error running {script}: {err}")
        print(f"Error output: {err.stderr}")
    except FileNotFoundError:
        print(f"✗ Script not found: {script}")
    finally:
        print("All done")

if __name__ == "__main__":
    main()