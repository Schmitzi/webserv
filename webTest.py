#!/usr/bin/env python3

import subprocess
import socket
import os
import time
import http.client
from threading import Thread
import mimetypes
import uuid
import json
import tempfile
import sys

WEBSERV_BINARY = "./webserv"
CONFIG_PATH = "configs/test.conf"
HOST = "127.0.0.1"
PORT = 8080

class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    RESET = '\033[0m'

def print_test(msg, status="INFO"):
    color = Colors.BLUE if status == "INFO" else Colors.GREEN if status == "PASS" else Colors.RED
    print(f"{color}[{status}]{Colors.RESET} {msg}")

def check_compilation():
    print_test("Checking compilation flags...")
    result = subprocess.run(["make", "re"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        print_test("Compilation failed", "FAIL")
        print(result.stderr.decode())
        return False
    else:
        print_test("Compilation passed", "PASS")
        return True

def start_server(config=None):
    config_file = config or CONFIG_PATH
    print_test(f"Starting server with config: {config_file}")
    try:
        proc = subprocess.Popen([WEBSERV_BINARY, config_file], 
                               stdout=subprocess.PIPE, 
                               stderr=subprocess.PIPE)
        time.sleep(2)  # Give server more time to start
        return proc
    except Exception as e:
        print_test(f"Failed to start server: {e}", "FAIL")
        return None

def stop_server(proc):
    if proc is None:
        return
    print_test("Stopping server...")
    proc.terminate()
    try:
        proc.wait(timeout=3)
    except subprocess.TimeoutExpired:
        proc.kill()

def test_connection(path="/", method="GET", body=None, headers=None, expect_status=None, 
                   check_body_contains=None, timeout=10):
    conn = http.client.HTTPConnection(HOST, PORT, timeout=timeout)
    try:
        conn.request(method, path, body=body, headers=headers or {})
        res = conn.getresponse()
        status = res.status
        reason = res.reason
        data = res.read().decode(errors='ignore')
        
        # Check status code if expected
        if expect_status and status != expect_status:
            print_test(f"{method} {path} expected status {expect_status}, got {status} {reason}", "FAIL")
            return None
        
        # Check body content if expected
        if check_body_contains and check_body_contains not in data:
            print_test(f"{method} {path} response does not contain expected content", "FAIL")
            return None

        print_test(f"{method} {path} -> {status} {reason}", "PASS")
        return res
    except Exception as e:
        print_test(f"{method} {path} failed: {e}", "FAIL")
        return None
    finally:
        conn.close()

def test_basic_http_methods():
    print_test("=== Testing Basic HTTP Methods ===")
    
    # Test GET
    test_connection("/", "GET", expect_status=200)
    test_connection("/index.html", "GET", expect_status=200)
    test_connection("/nonexistent.html", "GET", expect_status=404)
    
    # Test POST
    headers = {"Content-Type": "application/x-www-form-urlencoded"}
    body = "test=data"
    test_connection("/upload", "POST", body=body, headers=headers)
    
    # Test DELETE
    test_connection("/upload/testfile.txt", "DELETE")

def test_error_handling():
    print_test("=== Testing Error Handling ===")
    
    # Test various error codes
    test_connection("/nonexistent", "GET", expect_status=404)
    test_connection("/forbidden", "GET", expect_status=403)
    
    # Test method not allowed (if configured)
    test_connection("/", "PATCH", expect_status=405)
    
    # Test malformed request
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((HOST, PORT))
        sock.send(b"INVALID HTTP REQUEST\r\n\r\n")
        sock.close()
        print_test("Malformed request sent", "PASS")
    except Exception as e:
        print_test(f"Malformed request test failed: {e}", "FAIL")

def test_large_request_body():
    print_test("=== Testing Large Request Bodies ===")
    
    # Test request body size limits
    large_body = "x" * (2 * 1024 * 1024)  # 2MB
    headers = {"Content-Type": "text/plain", "Content-Length": str(len(large_body))}
    
    res = test_connection("/upload", "POST", body=large_body, headers=headers)
    if res and res.status == 413:
        print_test("Large request body correctly rejected with 413", "PASS")
    else:
        print_test("Large request body handling needs review", "FAIL")

def test_chunked_transfer():
    print_test("=== Testing Chunked Transfer Encoding ===")
    
    # Test chunked request
    headers = {"Transfer-Encoding": "chunked", "Content-Type": "text/plain"}
    
    # Manual chunked body
    chunk_data = "This is a chunked request"
    chunk_size = hex(len(chunk_data))[2:]
    chunked_body = f"{chunk_size}\r\n{chunk_data}\r\n0\r\n\r\n"
    
    test_connection("/upload", "POST", body=chunked_body, headers=headers)

def test_concurrent_connections():
    print_test("=== Testing Concurrent Connections ===")
    
    results = []
    def make_request(i):
        res = test_connection(f"/test_{i}", "GET")
        results.append(res is not None)
    
    threads = []
    for i in range(20):  # Test more concurrent connections
        t = Thread(target=make_request, args=(i,))
        t.start()
        threads.append(t)
    
    for t in threads:
        t.join()
    
    success_count = sum(results)
    print_test(f"Concurrent connections: {success_count}/20 successful", 
              "PASS" if success_count >= 15 else "FAIL")

def test_keep_alive():
    print_test("=== Testing Keep-Alive Connections ===")
    
    conn = http.client.HTTPConnection(HOST, PORT)
    try:
        # Make multiple requests on same connection
        for i in range(3):
            conn.request("GET", f"/test_{i}")
            res = conn.getresponse()
            data = res.read()
            if res.status != 200 and res.status != 404:
                print_test(f"Keep-alive request {i} failed", "FAIL")
                return
        
        print_test("Keep-alive connections working", "PASS")
    except Exception as e:
        print_test(f"Keep-alive test failed: {e}", "FAIL")
    finally:
        conn.close()

def test_multipart_upload():
    print_test("=== Testing Multipart File Upload ===")
    
    boundary = f"----WebKitFormBoundary{uuid.uuid4().hex[:16]}"
    filename = "test_upload.txt"
    file_content = "This is a test upload file.\nLine 2 of content.\n🚀 Unicode test"
    
    # Proper multipart/form-data body
    body_parts = [
        f"--{boundary}",
        f'Content-Disposition: form-data; name="file"; filename="{filename}"',
        "Content-Type: text/plain",
        "",
        file_content,
        f"--{boundary}--",
        ""
    ]
    body = "\r\n".join(body_parts)
    
    headers = {
        "Content-Type": f"multipart/form-data; boundary={boundary}",
        "Content-Length": str(len(body.encode()))
    }
    
    res = test_connection("/upload", "POST", body=body.encode(), headers=headers)
    if res and res.status in (200, 201):
        print_test("Multipart upload successful", "PASS")
    else:
        print_test("Multipart upload failed", "FAIL")

def test_cgi_functionality():
    print_test("=== Testing CGI Functionality ===")
    
    # Test different CGI scripts
    cgi_tests = [
        ("/cgi-bin/hello.py", "POST", {"name": "test"}),
        ("/cgi-bin/hello.php", "GET", {}),
        ("/cgi-bin/env.cgi", "GET", {}),
    ]
    
    for path, method, data in cgi_tests:
        if method == "POST":
            body = "&".join(f"{k}={v}" for k, v in data.items())
            headers = {"Content-Type": "application/x-www-form-urlencoded"}
            test_connection(path, method, body=body, headers=headers)
        else:
            test_connection(path, method)

def test_directory_listing():
    print_test("=== Testing Directory Listing ===")
    
    # Test autoindex on/off
    res = test_connection("/listing/", "GET")
    if res:
        data = res.read().decode(errors='ignore')
        if "Index of" in data or "Directory listing" in data:
            print_test("Directory listing enabled", "PASS")
        else:
            print_test("Directory listing may be disabled", "INFO")

def test_redirection():
    print_test("=== Testing HTTP Redirections ===")
    
    # Test various redirect codes
    redirect_tests = [
        "/redirect301",
        "/redirect302", 
        "/redirectme"
    ]
    
    for path in redirect_tests:
        res = test_connection(path, "GET")
        if res and res.status in (301, 302, 303, 307, 308):
            location = res.getheader("Location")
            if location:
                print_test(f"Redirect {path} -> {location}", "PASS")
            else:
                print_test(f"Redirect {path} missing Location header", "FAIL")

def test_mime_types():
    print_test("=== Testing MIME Type Detection ===")
    
    mime_tests = [
        ("/test.html", "text/html"),
        ("/test.css", "text/css"),
        ("/test.js", "application/javascript"),
        ("/test.jpg", "image/jpeg"),
        ("/test.png", "image/png"),
    ]
    
    for path, expected_mime in mime_tests:
        res = test_connection(path, "GET")
        if res:
            content_type = res.getheader("Content-Type")
            if content_type and expected_mime in content_type:
                print_test(f"MIME type for {path}: {content_type}", "PASS")
            else:
                print_test(f"MIME type for {path} incorrect: {content_type}", "FAIL")

def test_path_traversal_security():
    print_test("=== Testing Path Traversal Security ===")
    
    # Test path traversal attempts
    malicious_paths = [
        "/../../../etc/passwd",
        "/upload/../../../etc/passwd", 
        "/../config/default.conf",
        "/./././../etc/passwd"
    ]
    
    for path in malicious_paths:
        res = test_connection(path, "GET")
        if res and res.status == 403:
            print_test(f"Path traversal blocked: {path}", "PASS")
        elif res and res.status == 404:
            print_test(f"Path traversal handled: {path}", "PASS")
        else:
            print_test(f"Path traversal security concern: {path}", "FAIL")

def test_server_resilience():
    print_test("=== Testing Server Resilience ===")
    
    # Test rapid connection/disconnection
    for i in range(10):
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((HOST, PORT))
            sock.send(b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n")
            sock.close()  # Immediate close
            time.sleep(1);
        except:
            pass
    
    # Verify server is still responsive
    time.sleep(1)
    if test_connection("/", "GET", expect_status=200):
        print_test("Server resilience test passed", "PASS")
    else:
        print_test("Server may have issues with connection handling", "FAIL")

def test_http_headers():
    print_test("=== Testing HTTP Headers ===")
    
    res = test_connection("/", "GET")
    if res:
        # Check important headers
        server_header = res.getheader("Server")
        if server_header and "WebServ" in server_header:
            print_test(f"Server header: {server_header}", "PASS")
        
        content_length = res.getheader("Content-Length")
        if content_length:
            print_test(f"Content-Length header present: {content_length}", "PASS")

def test_different_ports():
    print_test("=== Testing Multiple Ports (if configured) ===")
    
    # Test different ports if your config has multiple
    ports_to_test = [8080, 8081, 8082]
    
    for port in ports_to_test:
        try:
            conn = http.client.HTTPConnection(HOST, port, timeout=3)
            conn.request("GET", "/")
            res = conn.getresponse()
            if res.status == 200:
                print_test(f"Port {port} responding", "PASS")
            conn.close()
        except:
            print_test(f"Port {port} not available or not configured", "INFO")

def test_stress_test():
    print_test("=== Stress Testing ===")
    
    def stress_worker():
        for _ in range(50):
            test_connection("/", "GET")
    
    threads = []
    for _ in range(5):
        t = Thread(target=stress_worker)
        t.start()
        threads.append(t)
    
    for t in threads:
        t.join()
    
    # Check if server is still responsive after stress
    if test_connection("/", "GET", expect_status=200):
        print_test("Stress test completed - server still responsive", "PASS")
    else:
        print_test("Server may have issues under stress", "FAIL")

def main():
    if not check_compilation():
        return
    
    server_proc = start_server()
    if not server_proc:
        return
    
    # Give server time to start properly
    time.sleep(3)
    
    try:
        # Test server is actually running
        if not test_connection("/", "GET"):
            print_test("Server not responding - check config and setup", "FAIL")
            return
        
        # Run all tests
        test_basic_http_methods()
        test_error_handling()
        test_multipart_upload()
        test_directory_listing()
        test_redirection()
        test_cgi_functionality()
        test_mime_types()
        test_path_traversal_security()
        test_http_headers()
        test_keep_alive()
        test_concurrent_connections()
        test_chunked_transfer()
        test_large_request_body()
        test_different_ports()
        test_server_resilience()
        test_stress_test()
        
        print_test("=== All tests completed ===")
        
    finally:
        stop_server(server_proc)

if __name__ == "__main__":
    main()
