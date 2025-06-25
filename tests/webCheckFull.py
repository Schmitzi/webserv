#!/usr/bin/env python3

import subprocess
import socket
import time
import http.client
from colorama import Fore, Style

WEBSERV_BINARY = "./webserv"
CONFIG_PATH = "config/test.conf"
HOST = "127.0.0.1"
PORT = 8080

PASS_COUNT = 0
TOTAL_COUNT = 0

def print_result(success, msg):
    global PASS_COUNT, TOTAL_COUNT
    TOTAL_COUNT += 1
    if success:
        PASS_COUNT += 1
        print(f"{Fore.GREEN}✅ {msg}{Style.RESET_ALL}")
    else:
        print(f"{Fore.RED}❌ {msg}{Style.RESET_ALL}")

def check_compilation():
    print("[*] Checking compilation flags...")
    result = subprocess.run(["make", "re"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        print_result(False, "Compilation failed")
    else:
        print_result(True, "Compilation PASS_COUNT")

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
            print_result(False, f"{method} {path} expected status {expect_status}, got {status} {reason}")
            return None

        if check_body_contains and check_body_contains not in data:
            print_result(False, f"{method} {path} response does not contain expected content.")
            return None

        print_result(True, f"{method} {path} -> {status} {reason}")
        return (status, res.getheaders(), data)
    except Exception as e:
        print_result(False, f"{method} {path} failed: {e}")
    finally:
        conn.close()

def test_head_method():
    print("[*] Testing HEAD method...")
    conn = http.client.HTTPConnection(HOST, PORT)
    try:
        conn.request("HEAD", "/index.html")
        res = conn.getresponse()
        data = res.read().decode(errors='ignore')
        if res.status == 200 and not data:
            print_result(True, "HEAD request successful and no body returned.")
        else:
            print_result(False, "HEAD request failed or body returned.")
    except Exception as e:
        print_result(False, f"HEAD request failed: {e}")
    finally:
        conn.close()

def test_options_method():
    print("[*] Testing OPTIONS method...")
    res = test_connection("/", "OPTIONS")
    if res:
        status, headers, _ = res
        allow = dict(headers).get("Allow", "")
        if "GET" in allow:
            print_result(True, f"OPTIONS returned Allow header: {allow}")
        else:
            print_result(False, "OPTIONS missing expected Allow header.")

def test_invalid_method():
    print("[*] Testing invalid HTTP method...")
    sock = None
    try:
        sock = socket.create_connection((HOST, PORT))
        sock.sendall(b"BREW / HTTP/1.1\r\nHost: localhost\r\n\r\n")
        data = sock.recv(1024).decode()
        if "400" in data:
            print_result(True, "Invalid method handled correctly.")
        else:
            print_result(False, "Invalid method response missing or incorrect.")
    except Exception as e:
        print_result(False, f"Invalid method test failed: {e}")
    finally:
        if sock:
            sock.close()

def test_large_post():
    print("[*] Testing large POST body...")
    large_body = "data=" + ("A" * (1024 * 1024 - 5))  # ~1MB form data
    headers = {"Content-Type": "application/x-www-form-urlencoded"}
    res = test_connection("/upload/large_post", "POST", body=large_body, headers=headers, expect_status=200)
    if res:
        print_result(True, "Large POST body test was successful.")
    else:
        print_result(False, "Large POST body not handled properly.")

def test_path_traversal():
    print("[*] Testing path traversal...")
    path = "/upload/../../etc/passwd"
    res = test_connection(path, "GET", expect_status=403)
    if res:
        print_result(True, "Path traversal blocked.")
    else:
        print_result(False, "Path traversal not handled properly.")

def test_query_parameters():
    print("[*] Testing query parameters...")
    res = test_connection("/cgi-bin/hello.py?foo=bar&baz=qux", "GET", expect_status=200)
    if res and "foo=bar" in res[2] and "baz=qux" in res[2]:
        print_result(True, "Query parameters handled correctly.")
    else:
        print_result(False, "Query parameters not handled properly.")

def test_keep_alive():
    print("[*] Testing keep-alive support...")
    conn = http.client.HTTPConnection(HOST, PORT)
    try:
        headers = {"Connection": "keep-alive"}
        conn.request("GET", "/", headers=headers)
        res1 = conn.getresponse()
        res1.read()
        conn.request("GET", "/", headers=headers)
        res2 = conn.getresponse()
        res2.read()
        if res1.status == 200 and res2.status == 200:
            print_result(True, "Keep-alive supported.")
        else:
            print_result(False, "Keep-alive not working.")
    except Exception as e:
        print_result(False, f"Keep-alive test failed: {e}")
    finally:
        conn.close()

def test_url_encoding():
    print("[*] Testing URL-encoded paths...")
    res = test_connection("/list/%69%6E%64%65%78%2E%68%74%6D%6C", "GET", expect_status=200)
    if res:
        print_result(True, "URL-encoded paths decoded and handled.")
    else:
        print_result(False, "URL-encoded paths failed.")

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
        test_url_encoding()
    finally:
        stop_server(server_proc)

    print("\n=====================================")
    print(f"SUMMARY: PASS {PASS_COUNT} / {TOTAL_COUNT}")
    print("=====================================")

if __name__ == "__main__":
    main()
