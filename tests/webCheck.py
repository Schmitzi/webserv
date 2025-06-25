#!/usr/bin/env python3

import subprocess
import time
import http.client
from threading import Thread
import uuid
from colorama import Fore, Style, init

# Initialize colorama for colored output
init(autoreset=True)

WEBSERV_BINARY = "./webserv"
CONFIG_PATH = "config/test.conf"
HOST = "127.0.0.1"
PORT = 8080

PASS_COUNT = 0
TOTAL_COUNT = 0

def success(msg): print(f"{Fore.GREEN}✅ {msg}{Style.RESET_ALL}")
def fail(msg): print(f"{Fore.RED}❌ {msg}{Style.RESET_ALL}")
def header(msg): print(f"\n===> {msg}")

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
        print(f"{Fore.RED}{e}{Style.RESET_ALL}")

def check_compilation():
    print("[*] Checking compilation flags...")
    result = subprocess.run(["make", "re"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        raise Exception("Compilation failed")
    else:
        print("✅ Compilation passed")

def start_server():
    print("[*] Starting server...")
    return subprocess.Popen([WEBSERV_BINARY, CONFIG_PATH], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

def stop_server(proc):
    print("[*] Stopping server...")
    proc.terminate()
    try:
        proc.wait(timeout=3)
    except subprocess.TimeoutExpired:
        proc.kill()

def test_connection(path="/", method="GET", body=None, headers=None, expect_status=None, check_body_contains=None):
    conn = http.client.HTTPConnection(HOST, PORT, timeout=5)
    try:
        conn.request(method, path, body=body, headers=headers or {})
        res = conn.getresponse()
        status = res.status
        reason = res.reason
        data = res.read().decode(errors='ignore')

        if expect_status and status != expect_status:
            raise Exception(f"{method} {path} expected status {expect_status}, got {status} {reason}")
        
        if check_body_contains and check_body_contains not in data:
            raise Exception(f"{method} {path} response does not contain expected content.")
        
        print(f"✅ {method} {path} -> {status} {reason}")
        return (status, res.getheaders(), data)
    except Exception as e:
        raise
    finally:
        conn.close()

def test_concurrent_clients(count=10):
    print(f"[*] Testing {count} concurrent clients...")
    threads = []
    errors = []
    
    def client_task():
        try:
            test_connection()
        except Exception as e:
            errors.append(str(e))
    
    for _ in range(count):
        t = Thread(target=client_task)
        t.start()
        threads.append(t)
    for t in threads:
        t.join()
    
    if errors:
        raise Exception(f"Concurrent clients test had errors: {errors}")
    print("✅ Concurrent connection test finished.")

def test_post_upload():
    print("[*] Testing simple upload (empty file creation)...")
    headers = {
        "Content-Type": "application/x-www-form-urlencoded"
    }
    body = "name=test&value=upload"
    # Expecting 200 or 201 (created) here depending on your server config
    test_connection("/upload", "POST", body=body, headers=headers, expect_status=200)

def test_post_upload_with_content():
    print("[*] Testing file upload with actual content (multipart/form-data)...")
    boundary = uuid.uuid4().hex
    filename = "test_upload.txt"
    file_content = "This is a test upload file.Line 2 of content."

    body_lines = [
        f"--{boundary}",
        f'Content-Disposition: form-data; name="file"; filename="{filename}"',
        "Content-Type: text/plain",
        "",
        file_content,
        f"--{boundary}--",
        ""
    ]
    body = "\r\n".join(body_lines)

    headers = {
        "Content-Type": f"multipart/form-data; boundary={boundary}",
        "Content-Length": str(len(body))
    }

    res = test_connection("/upload", "POST", body=body, headers=headers, expect_status=200)
    status, _, _ = res  # unpack result, guaranteed by test_connection if no exception
    # After upload, try to GET the uploaded file to verify content
    get_res = test_connection(f"/upload/{filename}", "GET", expect_status=200)
    _, _, data = get_res
    if file_content in data:
        print("✅ Uploaded file content verified successfully.")
    else:
        raise Exception("Uploaded file content mismatch or empty.")

def test_delete():
    print("[*] Testing DELETE method...")
    # Try deleting a known file (adjust filename if needed)
    res = test_connection("/upload/test", "DELETE")
    status, _, _ = res
    if status not in (200, 204):
        raise Exception(f"DELETE request returned unexpected status {status}")

def test_static_file():
    print("[*] Testing serving static file /index.html ...")
    res = test_connection("/index.html", "GET", expect_status=200, check_body_contains="<html")
    if res is None:
        raise Exception("Static file test failed.")

def test_directory_listing():
    print("[*] Testing directory listing at /listing/ ...")
    res = test_connection("/listing/", "GET", expect_status=200, check_body_contains="Index of")
    if res is None:
        raise Exception("Directory listing test failed.")

def test_redirection():
    print("[*] Testing redirection /redirectme ...")
    res = test_connection("/redirectme")
    status, headers, _ = res
    if status not in (301, 302):
        raise Exception(f"Redirection failed: unexpected status {status}")
    header_dict = dict(headers)
    location = header_dict.get("Location")
    if not location:
        raise Exception("Redirection header missing Location.")
    print(f"✅ Redirection works. Location: {location}")

def test_cgi():
    print("[*] Testing CGI script POST ...")
    headers = {"Content-Type": "application/x-www-form-urlencoded"}
    body = "foo=bar"
    res = test_connection("/cgi-bin/hello.py", "POST", body=body, headers=headers, expect_status=200)
    if not res:
        raise Exception("CGI script test failed.")

def main():
    check_compilation()

    server_proc = start_server()
    time.sleep(1)  # wait for server to start

    try:
        run_test("Static file test /index.html", test_static_file)
        run_test("Simple POST upload test", test_post_upload)
        run_test("Multipart file upload test", test_post_upload_with_content)
        run_test("DELETE method test", test_delete)
        run_test("Directory listing test", test_directory_listing)
        run_test("Redirection test", test_redirection)
        run_test("CGI POST test", test_cgi)
        run_test("Concurrent clients test", test_concurrent_clients)
    finally:
        stop_server(server_proc)
    
    print("\n=====================================")
    print(f"SUMMARY: PASS {PASS_COUNT} / {TOTAL_COUNT}")
    print("=====================================")

if __name__ == "__main__":
    main()
