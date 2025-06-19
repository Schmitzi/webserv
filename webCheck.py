#!/usr/bin/env python3

import subprocess
import socket
import os
import time
import http.client
from threading import Thread
import mimetypes
import uuid

WEBSERV_BINARY = "./webserv"
CONFIG_PATH = "config/test.conf"
HOST = "127.0.0.1"
PORT = 8080

def check_compilation():
    print("[*] Checking compilation flags...")
    result = subprocess.run(["make", "re"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        print("❌ Compilation failed")
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
        data = res.read().decode(errors='ignore')  # read ONCE here

        # Check status code if expected
        if expect_status and status != expect_status:
            print(f"❌ {method} {path} expected status {expect_status}, got {status} {reason}")
            return None
        
        # Check body content if expected
        if check_body_contains and check_body_contains not in data:
            print(f"❌ {method} {path} response does not contain expected content.")
            return None

        print(f"✅ {method} {path} -> {status} {reason}")
        return (status, res.getheaders(), data)  # return a tuple
    except Exception as e:
        print(f"❌ {method} {path} failed: {e}")
    finally:
        conn.close()

def test_concurrent_clients(count=10):
    print(f"[*] Testing {count} concurrent clients...")
    threads = []
    for _ in range(count):
        t = Thread(target=test_connection)
        t.start()
        threads.append(t)
    for t in threads:
        t.join()
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
    if res is not None:
        status, _, _ = res
        # After upload, try to GET the uploaded file to verify content
        get_res = test_connection(f"/upload/{filename}", "GET", expect_status=200)
        if get_res:
            _, _, data = get_res  # we already decoded it in test_connection
            print("Expected:", repr(file_content))
            print("Actual:", repr(data))
            if file_content in data:
                print("✅ Uploaded file content verified successfully.")
            else:
                print("❌ Uploaded file content mismatch or empty.")
        else:
            print("❌ Failed to retrieve uploaded file after upload.")
    else:
        print("❌ Upload request failed.")

def test_delete():
    print("[*] Testing DELETE method...")
    # Try deleting a known file (adjust filename if needed)
    res = test_connection("/upload/test", "DELETE")
    if res is not None:
        status, _, _ = res
        if status in (200, 204):
            print("✅ DELETE request succeeded.")
        else:
            print("❌ DELETE request failed or returned wrong status.")
    else:
        print("❌ DELETE request failed or file not found.")

def test_static_file():
    print("[*] Testing serving static file /index.html ...")
    res = test_connection("/index.html", "GET", expect_status=200, check_body_contains="<html")
    if res is not None:
        print("✅ Static file served correctly.")
    else:
        print("❌ Static file test failed.")

def test_directory_listing():
    print("[*] Testing directory listing at /listing/ ...")
    res = test_connection("/listing/", "GET", expect_status=200, check_body_contains="Index of")
    if res is not None:
        print("✅ Directory listing displayed correctly.")
    else:
        print("❌ Directory listing test failed.")

def test_redirection():
    print("[*] Testing redirection /redirectme ...")
    res = test_connection("/redirectme")
    if res:
        status, headers, _ = res
        if status in (301, 302):
            # headers is a list of tuples, convert to dict to access easily
            header_dict = dict(headers)
            location = header_dict.get("Location")
            if location:
                print(f"✅ Redirection works. Location: {location}")
            else:
                print("❌ Redirection header missing Location.")
        else:
            print(f"❌ Redirection failed: unexpected status {status}")
    else:
        print("❌ Redirection request failed.")

def test_cgi():
    print("[*] Testing CGI script POST ...")
    headers = {"Content-Type": "application/x-www-form-urlencoded"}
    body = "foo=bar"
    res = test_connection("/cgi-bin/hello.py", "POST", body=body, headers=headers, expect_status=200)
    if res:
        print("✅ CGI script executed successfully.")
    else:
        print("❌ CGI script test failed.")

def main():
    check_compilation()

    server_proc = start_server()
    time.sleep(1)  # wait for server to start

    try:
        test_static_file()
        test_post_upload()
        test_post_upload_with_content()
        test_delete()
        test_directory_listing()
        test_redirection()
        test_cgi()
        test_concurrent_clients()
    finally:
        stop_server(server_proc)

if __name__ == "__main__":
    main()
