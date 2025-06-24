#!/usr/bin/env python3

import subprocess
import socket
import time
import http.client

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
        data = res.read().decode(errors='ignore')

        if expect_status and status != expect_status:
            print(f"❌ {method} {path} expected status {expect_status}, got {status} {reason}")
            return None

        if check_body_contains and check_body_contains not in data:
            print(f"❌ {method} {path} response does not contain expected content.")
            return None

        print(f"✅ {method} {path} -> {status} {reason}")
        return (status, res.getheaders(), data)
    except Exception as e:
        print(f"❌ {method} {path} failed: {e}")
    finally:
        conn.close()

def test_head_method():
    print("[*] Testing HEAD method...")
    conn = http.client.HTTPConnection(HOST, PORT)
    conn.request("HEAD", "/index.html")
    res = conn.getresponse()
    data = res.read().decode(errors='ignore')
    if res.status == 200 and not data:
        print("✅ HEAD request successful and no body returned.")
    else:
        print("❌ HEAD request failed or body returned.")
    conn.close()

def test_options_method():
    print("[*] Testing OPTIONS method...")
    res = test_connection("/", "OPTIONS")
    if res:
        status, headers, _ = res
        allow = dict(headers).get("Allow", "")
        if "GET" in allow:
            print(f"✅ OPTIONS returned Allow header: {allow}")
        else:
            print("❌ OPTIONS missing expected Allow header.")

def test_invalid_method():
    print("[*] Testing invalid HTTP method...")
    try:
        sock = socket.create_connection((HOST, PORT))
        sock.sendall(b"BREW / HTTP/1.1\r\nHost: localhost\r\n\r\n")
        data = sock.recv(1024).decode()
        if "400" in data:
            print("✅ Invalid method handled correctly.")
        else:
            print("❌ Invalid method response missing or incorrect.")
    finally:
        sock.close()

def test_large_post():
    print("[*] Testing large POST body...")
    large_body = "A" * 1024 * 1024  # 1MB
    headers = {"Content-Type": "application/x-www-form-urlencoded"}
    test_connection("/upload", "POST", body=large_body, headers=headers)

def test_path_traversal():
    print("[*] Testing path traversal...")
    path = "/upload/../../etc/passwd"
    res = test_connection(path, "GET", expect_status=403)
    if res:
        print("✅ Path traversal blocked.")
    else:
        print("❌ Path traversal not handled properly.")

def test_query_parameters():
    print("[*] Testing query parameters...")
    res = test_connection("/cgi-bin/hello.py?foo=bar&baz=qux", "GET", expect_status=200)
    if res and "foo=bar" in res[2] and "baz=qux" in res[2]:
        print("✅ Query parameters handled correctly.")
    else:
        print("❌ Query parameters not handled properly.")

def test_keep_alive():
    print("[*] Testing keep-alive support...")
    conn = http.client.HTTPConnection(HOST, PORT)
    headers = {"Connection": "keep-alive"}
    conn.request("GET", "/", headers=headers)
    res1 = conn.getresponse()
    res1.read()
    conn.request("GET", "/", headers=headers)
    res2 = conn.getresponse()
    res2.read()
    conn.close()
    if res1.status == 200 and res2.status == 200:
        print("✅ Keep-alive supported.")
    else:
        print("❌ Keep-alive not working.")

def test_url_encoding():
    print("[*] Testing URL-encoded paths...")
    res = test_connection("/listing/%69%6E%64%65%78%2E%68%74%6D%6C", "GET", expect_status=200)
    if res:
        print("✅ URL-encoded paths decoded and handled.")
    else:
        print("❌ URL-encoded paths failed.")

def main():
    check_compilation()

    server_proc = start_server()
    time.sleep(1)

    try:
        test_connection("/", "GET", expect_status=200)
        # test_head_method()
        # test_options_method()
        test_invalid_method()
        test_large_post()
        test_path_traversal()
        test_query_parameters()
        test_keep_alive()
        test_url_encoding()#TODO: look up is needed?
    finally:
        stop_server(server_proc)

if __name__ == "__main__":
    main()
