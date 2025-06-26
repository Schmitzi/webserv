#!/usr/bin/env python3

import subprocess
import socket
import time
import http.client
from threading import Thread
import uuid
import tempfile
import os

WEBSERV_BINARY = "./webserv"
CONFIG_PATH = "config/test.conf"
HOST = "127.0.0.1"
PORT = 8080

class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    RESET = '\033[0m'

def print_test(msg, status=""):
    if status == "":
        print(f"-----[{msg}]-----")
    else:
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
        time.sleep(3)  # Give server more time to start
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
        proc.wait(timeout=5)
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
        
        if expect_status and status != expect_status:
            print_test(f"{method} {path} expected status {expect_status}, got {status} {reason}", "FAIL")
            return None, None
        
        if check_body_contains and check_body_contains not in data:
            print_test(f"{method} {path} response does not contain expected content", "FAIL")
            return None, None

        print_test(f"{method} {path} -> {status} {reason}", "PASS")
        return res, data
    except Exception as e:
        print_test(f"{method} {path} failed: {e}", "FAIL")
        return None, None
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
    body = "test=testfile.txt"
    test_connection("/upload", "POST", body=body, headers=headers)
    
    # Test DELETE
    test_connection("/upload/testfile.txt", "DELETE")

    print("\n")

def test_error_handling():
    print_test("=== Testing Error Handling ===")
    
    # Test various error codes
    test_connection("/nonexistent", "GET", expect_status=404)
    test_connection("/forbidden", "GET", expect_status=403)
    
    # Test malformed request
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((HOST, PORT))
        sock.send(b"INVALID HTTP REQUEST\r\n\r\n")
        time.sleep(0.1)  # Give server time to process
        sock.close()
        print_test("Malformed request sent", "PASS")
    except Exception as e:
        print_test(f"Malformed request test failed: {e}", "FAIL")
    print("\n")

def test_large_request_body():
    print_test("=== Testing Large Request Bodies ===")
    
    # First, test what your server's actual limit is
    test_sizes = [
        (1024 * 512, "512KB"),      # 512KB - should work  
        (1024 * 1024, "1MB"),       # 1MB - should work
        (1024 * 1024 * 2, "2MB"),   # 2MB - might work
        (1024 * 1024 * 5, "5MB"),   # 5MB - might hit limit
        (1024 * 1024 * 10, "10MB"), # 10MB - likely to fail
    ]
    
    print_test("Testing various request sizes to find server limit...")
    
    working_limit = 0
    failed_size = 0
    uploaded_files = []  # Track successfully uploaded files for cleanup
    
    for size_bytes, size_name in test_sizes:
        large_body = "x" * size_bytes
        headers = {"Content-Type": "text/plain", "Content-Length": str(len(large_body))}
        file_path = f"/upload/test_{size_name}.txt"
        
        print(f"  Testing {size_name} ({size_bytes:,} bytes)...")
        
        res, data = test_connection(file_path, "POST", 
                            body=large_body, headers=headers, timeout=60)
        
        if res:
            if res.status == 200 or res.status == 201:
                print_test(f"  {size_name}: ✅ ACCEPTED", "PASS")
                working_limit = size_bytes
                uploaded_files.append(file_path)  # Track for cleanup
            elif res.status == 413:
                print_test(f"  {size_name}: ⚠️  REJECTED (413 - too large)", "INFO")
                failed_size = size_bytes
                break
            elif res.status == 500:
                print_test(f"  {size_name}: ❌ SERVER ERROR (500)", "FAIL")
                break
            else:
                print_test(f"  {size_name}: ❓ UNEXPECTED ({res.status})", "INFO")
        else:
            print_test(f"  {size_name}: ❌ FAILED (timeout/error)", "FAIL")
            failed_size = size_bytes
            break
    
    # Clean up uploaded files using HTTP DELETE
    if uploaded_files:
        print_test(f"Cleaning up {len(uploaded_files)} uploaded test files...")
        
        cleanup_success = 0
        for file_path in uploaded_files:
            try:
                delete_res, data = test_connection(file_path, "DELETE", timeout=10)
                if delete_res and (delete_res.status == 200 or delete_res.status == 204):
                    cleanup_success += 1
                    print_test(f"  ✅ Deleted {file_path}", "INFO")
                elif delete_res and delete_res.status == 404:
                    print_test(f"  ⚠️  {file_path} not found (may have been auto-deleted)", "INFO")
                    cleanup_success += 1  # Count as success since file is gone
                else:
                    print_test(f"  ❌ Failed to delete {file_path} (status: {delete_res.status if delete_res else 'None'})", "INFO")
            except Exception as e:
                print_test(f"  ❌ Error deleting {file_path}: {e}", "INFO")
        
        if cleanup_success == len(uploaded_files):
            print_test("✅ All test files cleaned up successfully", "PASS")
        else:
            print_test(f"⚠️  Cleaned up {cleanup_success}/{len(uploaded_files)} files", "INFO")
    
    # Summary
    if working_limit > 0:
        working_mb = working_limit / (1024 * 1024)
        print_test(f"✅ Server accepts requests up to {working_mb:.1f}MB", "PASS")
        
        if working_limit >= 1024 * 1024:  # At least 1MB
            print_test("Server has reasonable upload limits", "PASS")
        else:
            print_test("Server upload limit might be too restrictive", "INFO")
    
    if failed_size > 0:
        failed_mb = failed_size / (1024 * 1024)
        print_test(f"⚠️  Server properly rejects requests larger than {failed_mb:.1f}MB", "PASS")
    
    # Test that server properly handles the rejection (not crash)
    if failed_size > 0:
        print_test("Testing server stability after large request rejection...")
        
        # Send a normal request to make sure server is still working
        normal_res, data = test_connection("/", "GET", expect_status=200)
        if normal_res:
            print_test("✅ Server remains stable after rejecting large request", "PASS")
        else:
            print_test("❌ Server may have issues after large request", "FAIL")

def test_request_size_edge_cases():
    print_test("=== Testing Request Size Edge Cases ===")
    
    # Test exactly at common limits
    common_limits = [
        (1024 * 1024, "1MB"),           # Common small limit
        (1024 * 1024 * 8, "8MB"),       # Common medium limit  
        (1024 * 1024 * 32, "32MB"),     # Common large limit
    ]
    
    uploaded_files = []  # Track files that need cleanup
    
    for size_bytes, size_name in common_limits:
        # Test just under the limit
        under_limit = size_bytes - 1024  # 1KB under
        under_body = "x" * under_limit
        under_headers = {"Content-Type": "text/plain", "Content-Length": str(len(under_body))}
        file_path = f"/upload/under_{size_name}.txt"
        
        print(f"Testing just under {size_name}...")
        res, data = test_connection(file_path, "POST", 
                            body=under_body, headers=under_headers, timeout=60)
        if res and (res.status == 200 or res.status == 201):
            print_test(f"  Just under {size_name}: ✅ ACCEPTED", "PASS")
            uploaded_files.append(file_path)  # Add to cleanup list
        elif res and res.status == 413:
            print_test(f"  Server limit is below {size_name}", "INFO")
            break
        else:
            if res:
                print(res.status)
            print_test(f"  Under {size_name}: ❓ Status {res.status if res else 'None'}", "INFO")
    
    # Clean up all uploaded files
    if uploaded_files:
        print_test(f"Cleaning up {len(uploaded_files)} edge case test files...")
        
        cleanup_success = 0
        for file_path in uploaded_files:
            try:
                delete_res, data = test_connection(file_path, "DELETE", timeout=60)
                if delete_res and (delete_res.status == 200 or delete_res.status == 204):
                    cleanup_success += 1
                    print_test(f"  🗑️  Deleted {file_path}", "INFO")
                elif delete_res and delete_res.status == 404:
                    print_test(f"  ⚠️  {file_path} not found (may have been auto-deleted)", "INFO")
                    cleanup_success += 1  # Count as success since file is gone
                else:
                    print_test(f"  ❌ Failed to delete {file_path} (status: {delete_res.status if delete_res else 'None'})", "INFO")
            except Exception as e:
                print_test(f"  ❌ Error deleting {file_path}: {e}", "INFO")
        
        if cleanup_success == len(uploaded_files):
            print_test("✅ All edge case test files cleaned up", "PASS")
        else:
            print_test(f"⚠️  Cleaned up {cleanup_success}/{len(uploaded_files)} edge case files", "INFO")
    
    print("\n")

def test_reasonable_upload_limits():
    print_test("=== Testing Reasonable Upload Handling ===")
    
    reasonable_size = 1024 * 1024  # 1MB
    reasonable_body = "x" * reasonable_size
    reasonable_headers = {"Content-Type": "text/plain", "Content-Length": str(len(reasonable_body))}
    file_path = "/upload/reasonable_test.txt"
    
    res, data = test_connection(file_path, "POST",
                        body=reasonable_body, headers=reasonable_headers, timeout=30)
    
    # Handle the response and cleanup
    try:
        if res and (res.status == 200 or res.status == 201):
            print_test("✅ Server accepts reasonable 1MB uploads", "PASS")
            
            # Clean up the uploaded file
            delete_res, data = test_connection(file_path, "DELETE", timeout=10)
            if delete_res and (delete_res.status in [200, 204, 404]):
                print_test("🗑️  Cleaned up test file", "INFO")
            else:
                print_test("⚠️  Could not clean up test file", "INFO")
            
            print("\n")
            return True
            
        elif res and res.status == 413:
            print_test("⚠️  Server rejects 1MB uploads (limit may be too low)", "INFO")
            # No file to clean up since upload was rejected
            print("\n")
            return False
            
        else:
            print_test(f"❓ Unexpected response for 1MB upload: {res.status if res else 'None'}", "FAIL")
            
            # Try to clean up in case file was partially created
            delete_res, data = test_connection(file_path, "DELETE", timeout=10)
            if delete_res and delete_res.status == 404:
                print_test("ℹ️  No file to clean up (upload failed)", "INFO")
            elif delete_res and (delete_res.status in [200, 204]):
                print_test("🗑️  Cleaned up partial file", "INFO")
            
            print("\n")
            return False
            
    except Exception as e:
        print_test(f"❌ Error during test: {e}", "FAIL")
        
        # Attempt cleanup even if there was an error
        try:
            delete_res, data = test_connection(file_path, "DELETE", timeout=10)
            if delete_res and (delete_res.status in [200, 204]):
                print_test("🗑️  Cleaned up after error", "INFO")
        except:
            pass  # Ignore cleanup errors
        
        print("\n")
        return False


def test_chunked_transfer():
    print_test("=== Testing Chunked Transfer Encoding ===")
    
    # Test chunked request
    headers = {"Transfer-Encoding": "chunked", "Content-Type": "text/plain"}
    
    # Manual chunked body
    chunk_data = "This is a chunked request"
    chunk_size = hex(len(chunk_data))[2:]
    chunked_body = f"{chunk_size}\r\n{chunk_data}\r\n0\r\n\r\n"
    
    test_connection("/upload/test", "POST", body=chunked_body, headers=headers)
    print("\n")

def test_concurrent_connections():
    print_test("=== Testing Concurrent Connections ===")
    
    results = []
    def make_request(i):
        res, data = test_connection(f"/upload/seahorse.jpg", "GET")
        results.append(res is not None)
    
    threads = []
    for i in range(10):
        t = Thread(target=make_request, args=(i,))
        t.start()
        threads.append(t)
    
    for t in threads:
        t.join()
    
    success_count = sum(results)
    print_test(f"Concurrent connections: {success_count}/10 successful", 
              "PASS" if success_count >= 8 else "FAIL")
    print("\n")

def test_keep_alive():
    print_test("=== Testing Keep-Alive Connections ===")
    
    conn = http.client.HTTPConnection(HOST, PORT)
    try:
        # Make multiple requests on same connection
        for i in range(3):
            conn.request("GET", f"/test_{i}")
            res = conn.getresponse()
            if res.status != 200 and res.status != 404:
                print_test(f"Keep-alive request {i} failed", "FAIL")
                return
        
        print_test("Keep-alive connections working", "PASS")
    except Exception as e:
        print_test(f"Keep-alive test failed: {e}", "FAIL")
    finally:
        conn.close()
    print("\n")

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
    
    res, data = test_connection("/upload", "POST", body=body.encode(), headers=headers)
    if res and res.status in (200, 201):
        print_test("Multipart upload successful", "PASS")
    else:
        print_test("Multipart upload failed", "FAIL")
    print("\n")


def test_cgi_functionality():
    print_test("=== Testing CGI Functionality ===")

    cgi_tests = [
        ("/cgi-bin/hello.py", "POST", {"name": "test"}),
        ("/cgi-bin/hello.php", "GET", {}),
        ("/cgi-bin/env.pl", "GET", {}),
    ]

    for path, method, data in cgi_tests:
        if method == "POST":
            body = "&".join(f"{k}={v}" for k, v in data.items())
            headers = {"Content-Type": "application/x-www-form-urlencoded"}
            res, data = test_connection(path, method, body=body, headers=headers)
        else:
            res, data = test_connection(path, method)

        if res:
            if res.status == 500:
                print_test(f"CGI script {path} returned 500 (Server Error)", "FAIL")
            elif res.status not in (200, 201):
                print_test(f"CGI script {path} returned unexpected status {res.status}", "FAIL")
        else:
            print_test(f"CGI script {path} test failed completely", "FAIL")
    print()

def test_directory_listing():
    print_test("=== Testing Directory Listing ===")

    res, data = test_connection("/listing/", "GET")
    if res and data:
        if "index of" in data.lower() or "<title>directory" in data.lower():
            print_test("Directory listing enabled", "PASS")
        elif res.status == 403:
            print_test("Directory listing disabled (403 Forbidden)", "PASS")
        elif res.status == 404:
            print_test("Directory not found (404)", "INFO")
        else:
            print_test("Directory listing status unclear", "FAIL")
    else:
        print_test("Directory listing request failed", "FAIL")
    print()


def test_redirection():
    print_test("=== Testing HTTP Redirections ===")

    redirect_tests = [
        ("/redirect301", 301),
        ("/redirect302", 302), 
        ("/redirectme", 302)
    ]

    for path, expected_code in redirect_tests:
        res, data = test_connection(path, "GET")
        if res and res.status == expected_code:
            location = res.getheader("Location")
            if location:
                print_test(f"Redirect {path} -> {location}", "PASS")
            else:
                print_test(f"Redirect {path} missing Location header", "FAIL")
        else:
            print_test(f"{path} expected redirect ({expected_code}), got {res.status if res else 'None'}", "FAIL")
    print()

def test_mime_types():
    print_test("=== Testing MIME Type Detection ===")
    
    mime_tests = [
        ("/test/test.html", "text/html"),
        ("/test/test.css", "text/css"),
        ("/test/test.js", "application/javascript"),
        ("/test/test.jpg", "image/jpeg"),
        ("/test/test.png", "image/png"),
    ]
    
    for path, expected_mime in mime_tests:
        res, data = test_connection(path, "GET")
        if res:
            content_type = res.getheader("Content-Type")
            if content_type and expected_mime in content_type:
                print_test(f"MIME type for {path}: {content_type}", "PASS")
            else:
                print_test(f"MIME type for {path} incorrect: {content_type}", "FAIL")
    print("\n")

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
        res, data = test_connection(path, "GET")
        if res and res.status == 403:
            print_test(f"Path traversal blocked: {path}", "PASS")
        elif res and res.status == 404:
            print_test(f"Path traversal handled: {path}", "PASS")
        else:
            print_test(f"Path traversal security concern: {path}", "FAIL")
    print("\n")

def test_server_resilience():
    print_test("=== Testing Server Resilience ===")
    
    successful_connections = 0
    
    for i in range(5):
        try:
            print(i)
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            sock.connect((HOST, PORT))
            
            # Send proper HTTP request
            request = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
            sock.send(request.encode())
            
            # Actually wait for response
            response = sock.recv(1024)
            if b"HTTP/1.1" in response:
                successful_connections += 1
            
            sock.close()
            time.sleep(2)  # Increased delay between connections
            
        except Exception as e:
            print_test(f"Connection {i} failed: {e}", "INFO")
            time.sleep(1)
    
    # Verify server is still responsive
    time.sleep(2)
    if test_connection("/", "GET", expect_status=200):
        print_test(f"Server resilience test: {successful_connections}/5 connections successful", "PASS")
    else:
        print_test("Server may have issues with connection handling", "FAIL")
    print("\n")

def test_http_headers():
    print_test("=== Testing HTTP Headers ===")
    
    res, data = test_connection("/", "GET")
    if res:
        # Check important headers
        server_header = res.getheader("Server")
        if server_header and "WebServ" in server_header:
            print_test(f"Server header: {server_header}", "PASS")
        
        content_length = res.getheader("Content-Length")
        if content_length:
            print_test(f"Content-Length header present: {content_length}", "PASS")
    print("\n")

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
    print("\n")

def test_stress_test():
    print_test("=== Stress Testing ===")
    
    def gentle_stress_worker():
        for _ in range(10):
            test_connection("/", "GET")
            time.sleep(0.1)
    
    threads = []
    for _ in range(3):
        t = Thread(target=gentle_stress_worker)
        t.start()
        threads.append(t)
    
    for t in threads:
        t.join()
    
    # Check if server is still responsive after stress
    time.sleep(3)  # Give server time to recover
    if test_connection("/", "GET", expect_status=200):
        print_test("Stress test completed - server still responsive", "PASS")
    else:
        print_test("Server may have issues under stress", "FAIL")

def main():
    print_test("=== Beginning Tests ===")
    try:
        # Test server is actually running
        if not test_connection("/", "GET"):
            print_test("Server not responding - check config and setup", "FAIL")
            return
        
        FORBIDDEN_DIR = tempfile.mkdtemp()
        os.chmod(FORBIDDEN_DIR, 0o000)
        WEB_ROOT = "./local/"
        linked_dir = os.path.join(WEB_ROOT, "forbidden")
        if os.path.islink(linked_dir) or os.path.exists(linked_dir):
            os.remove(linked_dir)
        os.symlink(FORBIDDEN_DIR, linked_dir)
        # Run all tests in order of severity (least to most stressful)
        test_basic_http_methods()
        test_error_handling()
        test_mime_types()
        test_http_headers()
        test_multipart_upload()
        test_directory_listing()
        test_redirection()
        test_cgi_functionality()
        test_path_traversal_security()
        test_keep_alive()
        test_chunked_transfer()
        test_large_request_body()
        test_request_size_edge_cases()
        test_reasonable_upload_limits()
        test_different_ports()
        test_concurrent_connections()
        test_server_resilience()
        test_stress_test()
        
        print_test("=== All tests completed ===")
        
    finally:
        os.chmod(FORBIDDEN_DIR, 0o700)
        os.remove(linked_dir)
        os.rmdir(FORBIDDEN_DIR)

if __name__ == "__main__":
    main()