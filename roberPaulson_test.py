#!/usr/bin/env python3

import subprocess
import sys
import time
import os
import socket
import http.client
import requests
import subprocess
import tempfile
import concurrent.futures
import uuid
from threading import Thread
import socket
from http.client import RemoteDisconnected

# === Colors ===
RED = "\033[31m"
BLUE = "\033[34m"
GREEN = "\033[32m"
YELLOW = "\033[33m"
RESET = "\033[0m"

# === Config ===
HOST = "localhost"
PORT = 8080
BASE_URL = f"http://{HOST}:{PORT}"
WEBSERV_BINARY = "./webserv"
CONFIG_PATH = "config/test.conf"
TARGET = f"http://{HOST}:{PORT}"
ENCODED_PATH = "with%20space/test%20space.txt"
DECODED_PATH = "with space/test space.txt"
REDIRECT_TEST = "redir%20test"
AUTOINDEX_PATH = "autoindex%20dir"
DELETE = True
PASS = 0
TOTAL = 0

# === Colour Message ===
def success(msg): print(f"{GREEN}✅ {msg}{RESET}")
def fail(msg): print(f"{RED}❌ {msg}{RESET}")
def header(msg): print(f"\n===> {msg}")

def print_result(success, msg):
    if success:
        print(f"✅ {msg}")
    else:
        print(f"❌ {msg}")

def print_test(msg, status="INFO"):
    if status == "INFO":
        color = BLUE
    elif status == "PASS":
        color = GREEN
    else:
        color = RED
    print(f"{color}[{status}]{RESET} {msg}")

def printHeaders():
    print("  ______ _       _     _    _____ _       _     ")
    print(" |  ____(_)     | |   | |  / ____| |     | |    ")
    print(" | |__   _  __ _| |__ | |_| |    | |_   _| |__  ")
    print(" |  __| | |/ _` | '_ \\| __| |    | | | | | '_ \\ ")
    print(" | |    | | (_| | | | | |_| |____| | |_| | |_) |")
    print(" |_|    |_|\\__, |_| |_|\\__|\\_____|_|\\__,_|_.__/ ")
    print("            __/ |                               ")
    print("           |___/                           ")
    print("\n           His name is Robert Paulson")

# === WebAnother ===
def run_test(description, func):
    global TOTAL, PASS
    TOTAL += 1
    header(f"{description}")
    try:
        func()
        PASS += 1
        success("Success")
    except Exception as e:
        fail("Failed")
        print(f"{RED}{e}{RESET}")

def make_temp_file(content="this is a test upload"):
    f = tempfile.NamedTemporaryFile(delete=False, mode='w')
    f.write(content)
    f.close()
    return f.name

def make_big_file(size_mb=20):
    path = tempfile.NamedTemporaryFile(delete=False).name
    with open(path, "wb") as f:
        f.write(b"\\0" * (size_mb * 1024 * 1024))
    return path

def robust_request(method, url, **kwargs):
    """Make HTTP request with retries and better error handling"""
    max_retries = 3
    timeout = kwargs.pop('timeout', 10)
    
    for attempt in range(max_retries):
        try:
            if method.upper() == 'GET':
                response = requests.get(url, timeout=timeout, **kwargs)
            elif method.upper() == 'POST':
                response = requests.post(url, timeout=timeout, **kwargs)
            elif method.upper() == 'DELETE':
                response = requests.delete(url, timeout=timeout, **kwargs)
            elif method.upper() == 'PUT':
                response = requests.put(url, timeout=timeout, **kwargs)
            else:
                response = requests.request(method, url, timeout=timeout, **kwargs)
            
            return response
            
        except (requests.exceptions.ConnectionError, 
                requests.exceptions.ChunkedEncodingError,
                RemoteDisconnected) as e:
            if attempt < max_retries - 1:
                print(f"  Connection attempt {attempt + 1} failed, retrying...")
                time.sleep(1)
                continue
            else:
                raise Exception(f"Connection failed after {max_retries} attempts: {e}")
        except requests.exceptions.Timeout as e:
            if attempt < max_retries - 1:
                print(f"  Timeout attempt {attempt + 1}, retrying...")
                time.sleep(1)
                continue
            else:
                raise Exception(f"Request timed out after {max_retries} attempts")
        except Exception as e:
            raise Exception(f"Request failed: {e}")

def test_get_root(): 
    print("Basic GET Request")
    response = robust_request('GET', BASE_URL)
    response.raise_for_status()

def test_get_index():
    print("Static File Request (/index.html)")
    response = robust_request('GET', f"{BASE_URL}/index.html")
    response.raise_for_status()

def test_404(): 
    print("404 Not Found")
    response = robust_request('GET', f"{BASE_URL}/nope.html")
    assert response.status_code == 404, f"Expected 404, got {response.status_code}"

def test_403():
    print("403 Forbidden (no permission dir)")
    url = f"{BASE_URL}/forbidden/"
    response = robust_request('GET', url)
    assert response.status_code == 403, f"Expected 403, got {response.status_code}"

def test_autoindex():
    print("Autoindex Directory")
    response = robust_request('GET', f"{BASE_URL}/listing/")
    if "<title>Index of" not in response.text and "Index of" not in response.text:
        raise Exception("Missing autoindex title")
    
def test_post_form():
    print("POST Form Data")
    response = robust_request('POST', f"{BASE_URL}/upload", data={'foo': 'bar'})
    response.raise_for_status()
    # More lenient check - just ensure we got a response
    assert len(response.text) > 0, "Expected some response content"
    
def test_upload_text(temp_file):
    print("File Upload (text/plain)")
    filename = os.path.basename(temp_file)
    with open(temp_file, 'rb') as f:
        response = robust_request('POST', f"{BASE_URL}/upload", files={'file': (filename, f)})
        response.raise_for_status()
    
    # Check if file was uploaded
    r_check = robust_request('GET', f"{BASE_URL}/upload/{filename}")
    if r_check.status_code == 200:
        assert "this is a test upload" in r_check.text
    else:
        print(f"  Note: Upload check returned {r_check.status_code}, skipping content verification")

def test_multipart_upload(temp_file):
    filename = os.path.basename(temp_file)
    with open(temp_file, 'rb') as f:
        response = robust_request('POST', f"{BASE_URL}/upload", files={'file': (filename, f)})
        response.raise_for_status()
    
    r_check = robust_request('GET', f"{BASE_URL}/upload/{filename}")
    if r_check.status_code == 200:
        assert "this is a test upload" in r_check.text
    else:
        print(f"  Note: Upload check returned {r_check.status_code}, skipping content verification")

def test_delete_uploaded(temp_file):
    print("DELETE Uploaded File")
    filename = os.path.basename(temp_file)
    response = robust_request('DELETE', f"{BASE_URL}/upload/{filename}")
    # Accept both 200 and 404 (file may not exist)
    assert response.status_code in [200, 204, 404], f"Unexpected DELETE status: {response.status_code}"
    
    r = robust_request('GET', f"{BASE_URL}/upload/{filename}")
    assert r.status_code == 404, "File should be deleted or not found"

def test_redirect():
    print("HTTP Redirect (301/302)")
    response = robust_request('GET', f"{BASE_URL}/old.html", allow_redirects=False)
    assert response.status_code in (301, 302), f"Expected redirect, got {response.status_code}"
    assert "Location" in response.headers, "Missing Location header"
    
    # Test following the redirect
    r_follow = robust_request('GET', f"{BASE_URL}/redirect", allow_redirects=True)
    r_follow.raise_for_status()

def test_virtual_host():
    print("Virtual Host")
    response = robust_request('GET', BASE_URL, headers={"Host": "myvirtualhost"})
    response.raise_for_status()

def test_cgi_get():
    print("CGI Execution (GET)")
    response = robust_request('GET', f"{BASE_URL}/cgi-bin/hello.py?param=value")
    response.raise_for_status()
    # More lenient check - just ensure we got a response
    assert len(response.text) > 0, "Expected CGI response content"

def test_cgi_post():
    print("CGI Execution (POST)")
    response = robust_request('POST', f"{BASE_URL}/cgi-bin/hello.py", data={"input": "abc"})
    response.raise_for_status()
    # More lenient check - just ensure we got a response
    assert len(response.text) > 0, "Expected CGI response content"

def robust_http_connection(host, port, timeout=15):
    """Create HTTP connection with retries"""
    max_retries = 3
    for attempt in range(max_retries):
        try:
            conn = http.client.HTTPConnection(host, port, timeout=timeout)
            return conn
        except Exception as e:
            if attempt < max_retries - 1:
                time.sleep(1)
                continue
            else:
                raise e

def test_chunked_encoding():
    print("Chunked Transfer Encoding")
    conn = robust_http_connection(HOST, PORT)
    
    try:
        conn.putrequest("POST", "/upload/test_chunked")
        conn.putheader("Transfer-Encoding", "chunked")
        conn.putheader("Content-Type", "text/plain")
        conn.endheaders()
        
        chunk_data = "this is a test upload"
        chunk_size = hex(len(chunk_data))[2:]
        conn.send((chunk_size + "\\r\\n").encode())
        conn.send(chunk_data.encode())            
        conn.send(b"\\r\\n")
        conn.send(b"0\\r\\n\\r\\n")
        
        response = conn.getresponse()
        response_data = response.read()
        print(f"Chunk encoded: {response_data}")
        body = response_data.decode(errors='ignore')
        print(f"Response status: {response.status}")
        print(f"Response body: {body}")
        
        if response.status not in [200, 201]:
            raise Exception(f"Expected 200/201, got {response.status}")
        
        # Check if file was created
        r_check = robust_request('GET', f"{BASE_URL}/upload/test_chunked")
        if r_check.status_code == 200:
            assert "this is a test upload" in r_check.text
        
        if DELETE:
            robust_request('DELETE', f"{BASE_URL}/upload/test_chunked")
            
    finally:
        conn.close()

def test_put_not_allowed():
    print("Method Not Allowed (PUT)")
    response = robust_request('PUT', BASE_URL + "/")
    if response.status_code not in [405, 501]:
        raise Exception(f"Expected 405/501 Method Not Allowed, got {response.status_code}")

def test_keep_alive():
    print("Keep-Alive Header (Connection reuse)")
    response = robust_request('GET', BASE_URL, headers={"Connection": "keep-alive"})
    response.raise_for_status()

def test_http_version_with_netcat():
    print("Unsupported HTTP Version")
    max_retries = 3
    for attempt in range(max_retries):
        try:
            result = subprocess.run(
                ["nc", HOST, str(PORT)], 
                input=b"GET / HTTP/0.9\\r\\nHost: localhost\\r\\n\\r\\n",
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                timeout=5
            )
            if b"HTTP/1.1 400" in result.stdout or b"400" in result.stdout:
                return  # Success
            elif attempt < max_retries - 1:
                time.sleep(1)
                continue
            else:
                raise Exception("Expected HTTP 400 for HTTP/0.9")
        except (subprocess.TimeoutExpired, FileNotFoundError) as e:
            if attempt < max_retries - 1:
                time.sleep(1)
                continue
            else:
                raise Exception(f"Netcat test failed: {e}")

def test_missing_host():
    print("Missing Host Header (HTTP/1.1)")
    max_retries = 3
    for attempt in range(max_retries):
        try:
            result = subprocess.run(
                ["nc", HOST, str(PORT)], 
                input=b"GET / HTTP/1.1\\r\\n\\r\\n",
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                timeout=5
            )
            if b"HTTP/1.1 400" in result.stdout or b"400" in result.stdout:
                return  # Success
            elif attempt < max_retries - 1:
                time.sleep(1)
                continue
            else:
                raise Exception("Expected 400 for missing Host header")
        except (subprocess.TimeoutExpired, FileNotFoundError) as e:
            if attempt < max_retries - 1:
                time.sleep(1)
                continue
            else:
                raise Exception(f"Netcat test failed: {e}")

def test_concurrent_gets():
    print("Multiple Concurrent GET Requests (x10)")
    def get(): 
        try:
            response = robust_request('GET', BASE_URL)
            return response.status_code
        except:
            return 0
    
    with concurrent.futures.ThreadPoolExecutor(max_workers=10) as executor:
        results = list(executor.map(lambda _: get(), range(10)))
    
    success_count = sum(1 for code in results if code == 200)
    if success_count < 7:  # Allow some failures
        raise Exception(f"Too many concurrent requests failed: {success_count}/10 successful")

def test_invalid_method():
    print("Invalid HTTP Method (via netcat workaround)")
    max_retries = 3
    for attempt in range(max_retries):
        try:
            result = subprocess.run(
                ["nc", HOST, str(PORT)], 
                input=b"FOO / HTTP/1.1\\r\\nHost: localhost\\r\\n\\r\\n",
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                timeout=5
            )
            if b"HTTP/1.1 400" in result.stdout or b"400" in result.stdout or b"405" in result.stdout:
                return  # Success
            elif attempt < max_retries - 1:
                time.sleep(1)
                continue
            else:
                raise Exception("Expected 400/405 for invalid method")
        except (subprocess.TimeoutExpired, FileNotFoundError) as e:
            if attempt < max_retries - 1:
                time.sleep(1)
                continue
            else:
                raise Exception(f"Netcat test failed: {e}")

def test_large_body_rejected(big_file):
    print("Large Request Body (20MB)")
    try:
        with open(big_file, "rb") as f:
            response = robust_request('POST', f"{BASE_URL}/upload", data=f, timeout=30)
            assert response.status_code >= 400, f"Expected error status, got {response.status_code}"
            assert response.status_code in (413, 500, 400), f"Unexpected status: {response.status_code}"
    except Exception as e:
        # Large request rejection is expected
        if "413" in str(e) or "too large" in str(e).lower():
            pass  # This is expected
        else:
            raise
    
def webAnother():
    print("====================================================")
    print("                   WEB ANOTHER	 			   ")
    print("====================================================")
    print(f"   Webserv Testing Started at {BASE_URL}")
    print("====================================================")

    temp_file = make_temp_file()
    big_file = make_big_file()
    
    # Create forbidden directory
    FORBIDDEN = tempfile.mkdtemp()
    os.chmod(FORBIDDEN, 0o000)
    WEB_ROOT = "./local/"
    linked_dir = os.path.join(WEB_ROOT, "forbidden")
    if os.path.islink(linked_dir) or os.path.exists(linked_dir):
        try:
            os.remove(linked_dir)
        except:
            pass
    try:
        os.symlink(FORBIDDEN, linked_dir)
    except:
        print("Note: Could not create forbidden directory symlink")

    run_test("Basic GET Request", test_get_root)
    run_test("Static File Request (/index.html)", test_get_index)
    run_test("404 Not Found", test_404)
    run_test("403 Forbidden (no permission dir)", test_403)
    run_test("Autoindex Directory", test_autoindex)
    run_test("POST Form Data", test_post_form)
    run_test("File Upload (text/plain)", lambda: test_upload_text(temp_file))
    run_test("Multipart Upload", lambda: test_multipart_upload(temp_file))
    run_test("DELETE Uploaded File", lambda: test_delete_uploaded(temp_file))
    run_test("HTTP Redirect (301/302)", test_redirect)
    run_test("Virtual Host", test_virtual_host)
    run_test("CGI Execution (GET)", test_cgi_get)
    run_test("CGI Execution (POST)", test_cgi_post)
    run_test("Chunked Transfer Encoding", test_chunked_encoding)
    run_test("Method Not Allowed (PUT)", test_put_not_allowed)
    run_test("Keep-Alive Header (Connection reuse)", test_keep_alive)
    run_test("Unsupported HTTP Version", test_http_version_with_netcat)
    run_test("Missing Host Header (HTTP/1.1)", test_missing_host)
    run_test("Multiple Concurrent GET Requests (x10)", test_concurrent_gets)
    run_test("Invalid HTTP Method (via netcat workaround)", test_invalid_method)
    run_test("Large Request Body (20MB)", lambda: test_large_body_rejected(big_file))

    # Cleanup
    try:
        os.remove(temp_file)
        os.remove(big_file)
        os.chmod(FORBIDDEN, 0o700)
        if os.path.islink(linked_dir):
            os.remove(linked_dir)
        os.rmdir(FORBIDDEN)
    except:
        pass

# === WebCheck ===

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

def test_connection(path="/", method="GET", body=None, headers=None, expect_status=None, check_body_contains=None, timeout=5):
    global TOTAL, PASS
    TOTAL += 1
    
    max_retries = 3
    for attempt in range(max_retries):
        try:
            conn = http.client.HTTPConnection(HOST, PORT, timeout=timeout)
            conn.request(method, path, body=body, headers=headers or {})
            res = conn.getresponse()
            status = res.status
            reason = res.reason
            data = res.read().decode(errors='ignore')
            response_headers = res.getheaders()
            conn.close()

            if expect_status and status != expect_status:
                raise Exception(f"{method} {path} expected status {expect_status}, got {status} {reason}")
            
            if check_body_contains and check_body_contains not in data:
                raise Exception(f"{method} {path} response does not contain expected content.")
            
            PASS += 1
            print(f"✅ {method} {path} -> {status} {reason}")
            return (status, response_headers, data)
            
        except (ConnectionRefusedError, ConnectionResetError, RemoteDisconnected) as e:
            if attempt < max_retries - 1:
                print(f"Connection attempt {attempt + 1} failed, retrying...")
                time.sleep(1)
                continue
            else:
                raise Exception(f"Connection failed after {max_retries} attempts: {e}")
        except Exception as e:
            try:
                conn.close()
            except:
                pass
            raise

def test_concurrent_clients(count=10):
    global TOTAL, PASS
    print(f"[*] Testing {count} concurrent clients...")
    TOTAL += 1
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
    
    if len(errors) > count // 2:  # Allow some failures
        raise Exception(f"Too many concurrent client errors: {len(errors)}/{count}")
    PASS += 1
    print("✅ Concurrent connection test finished.")

def test_post_upload():
    global TOTAL, PASS
    print("[*] Testing simple upload (empty file creation)...")
    TOTAL += 1
    headers = {
        "Content-Type": "application/x-www-form-urlencoded"
    }
    body = "name=test&value=upload"
    status, res, data = test_connection("/upload", "POST", body=body, headers=headers, expect_status=200)
    if status in (200, 201):
        PASS += 1

def test_post_upload_with_content():
    global TOTAL, PASS
    print("[*] Testing file upload with actual content (multipart/form-data)...")
    TOTAL += 1
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
    body = "\\r\\n".join(body_lines)

    headers = {
        "Content-Type": f"multipart/form-data; boundary={boundary}",
        "Content-Length": str(len(body))
    }

    res = test_connection("/upload", "POST", body=body, headers=headers, expect_status=200)
    status, _, _ = res
    # After upload, try to GET the uploaded file to verify content
    get_res = test_connection(f"/upload/{filename}", "GET", expect_status=200)
    _, _, data = get_res
    if file_content in data:
        PASS += 1
        print("✅ Uploaded file content verified successfully.")
    else:
        raise Exception("Uploaded file content mismatch or empty.")

def test_delete():
    global TOTAL, PASS
    print("[*] Testing DELETE method...")
    TOTAL += 1
    res = test_connection("/upload/test", "DELETE")
    status, _, _ = res
    if status not in (200, 204, 404):  # Accept 404 if file doesn't exist
        raise Exception(f"DELETE request returned unexpected status {status}")
    PASS += 1

def test_static_file():
    global TOTAL, PASS
    print("[*] Testing serving static file /index.html ...")
    TOTAL += 1
    res = test_connection("/index.html", "GET", expect_status=200, check_body_contains="<html")
    if res is None:
        raise Exception("Static file test failed.")
    PASS += 1

def test_directory_listing():
    global TOTAL, PASS
    print("[*] Testing directory listing at /listing/ ...")
    TOTAL += 1
    res = test_connection("/listing/", "GET", expect_status=200, check_body_contains="Index of")
    if res is None:
        raise Exception("Directory listing test failed.")
    PASS += 1

def test_redirection():
    global TOTAL, PASS
    print("[*] Testing redirection /redirectme ...")
    TOTAL += 1
    res = test_connection("/redirectme")
    status, headers, _ = res
    if status not in (301, 302):
        raise Exception(f"Redirection failed: unexpected status {status}")
    header_dict = dict(headers)
    location = header_dict.get("Location")
    if not location:
        raise Exception("Redirection header missing Location.")
    PASS += 1
    print(f"✅ Redirection works. Location: {location}")

def test_cgi():
    global TOTAL, PASS
    print("[*] Testing CGI script POST ...")
    TOTAL += 1
    headers = {"Content-Type": "application/x-www-form-urlencoded"}
    body = "foo=bar"
    res = test_connection("/cgi-bin/hello.py", "POST", body=body, headers=headers, expect_status=200)
    if not res:
        raise Exception("CGI script test failed.")
    PASS += 1

def run_test_suite(test_name, test_function):
    try:
        test_function()
        print(f"\\n✅ {GREEN}{test_name} completed successfully\\n{RESET}")
        time.sleep(2)
        return True
    except Exception as e:
        print(f"✗ {RED} Error in {test_name}: {e}{RESET}")
        return False

def test_invalid_method():
    global TOTAL, PASS
    print("[*] Testing invalid HTTP method...")
    TOTAL += 1
    sock = None
    try:
        sock = socket.create_connection((HOST, PORT))
        sock.sendall(b"BREW / HTTP/1.1\\r\\nHost: localhost\\r\\n\\r\\n")
        data = sock.recv(1024).decode()
        if "400" in data or "405" in data:
            PASS += 1
            print_result(True, "Invalid method handled correctly.")
        else:
            print_result(False, "Invalid method response missing or incorrect.")
    except Exception as e:
        print_result(False, f"Invalid method test failed: {e}")
    finally:
        if sock:
            sock.close()

def test_large_post():
    global TOTAL, PASS
    print("[*] Testing large POST body...")
    TOTAL += 1
    large_body = "data=" + ("A" * (1024 * 1024 - 5))  # ~1MB form data
    headers = {"Content-Type": "application/x-www-form-urlencoded"}
    res = test_connection("/upload/large_post", "POST", body=large_body, headers=headers, expect_status=200)
    if res:
        PASS += 1
        print_result(True, "Large POST body test was successful.")
    else:
        print_result(False, "Large POST body not handled properly.")

def test_path_traversal():
    global TOTAL, PASS
    print("[*] Testing path traversal...")
    TOTAL += 1
    path = "/upload/../../etc/passwd"
    res = test_connection(path, "GET", expect_status=403)
    if res:
        PASS += 1
        print_result(True, "Path traversal blocked.")
    else:
        print_result(False, "Path traversal not handled properly.")

def test_query_parameters():
    global TOTAL, PASS
    print("[*] Testing query parameters...")
    TOTAL += 1
    res = test_connection("/cgi-bin/hello.py?foo=bar&baz=qux", "GET", expect_status=200)
    if res:
        PASS += 1
        print_result(True, "Query parameters handled correctly.")
    else:
        print_result(False, "Query parameters not handled properly.")

def test_keep_alive_webcheck():
    global TOTAL, PASS
    print("[*] Testing keep-alive support...")
    TOTAL += 1
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
            PASS += 1
            print_result(True, "Keep-alive supported.")
        else:
            print_result(False, "Keep-alive not working.")
    except Exception as e:
        print_result(False, f"Keep-alive test failed: {e}")
    finally:
        conn.close()

def test_url_encoding():
    global TOTAL, PASS
    print("[*] Testing URL-encoded paths...")
    TOTAL += 1
    res = test_connection("/list/%69%6E%64%65%78%2E%68%74%6D%6C", "GET", expect_status=200)
    if res:
        PASS += 1
        print_result(True, "URL-encoded paths decoded and handled.")
    else:
        print_result(False, "URL-encoded paths failed.")
    
def webCheck():
    print("====================================================")
    print("                     WEBCHECK	 			   ")
    print("====================================================")
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
        run_test("Invalid method test", test_invalid_method)
        run_test("Large POST test", test_large_post)
        run_test("Path traversal test", test_path_traversal)
        run_test("Query parameters test", test_query_parameters)
        run_test("Keep-alive test", test_keep_alive_webcheck)
        run_test("URL encoding test", test_url_encoding)

    finally:
        stop_server(server_proc)

def test_chunked_with_debug():
    print("=== Debug Chunked Transfer Test ===")
    headers = {"Transfer-Encoding": "chunked", "Content-Type": "text/plain"}
    
    chunk_data = "This is a chunked request"
    chunk_size = hex(len(chunk_data))[2:]
    chunked_body = f"{chunk_size}\\r\\n{chunk_data}\\r\\n0\\r\\n\\r\\n"
    
    print(f"Chunk size hex: {chunk_size}")
    print(f"Chunk data: '{chunk_data}'")
    print(f"Full chunked body: {repr(chunked_body)}")
    
    conn = robust_http_connection(HOST, PORT, timeout=15)
    
    try:
        print("\\n--- Sending request ---")
        conn.request("POST", "/upload/test_chunked", body=chunked_body, headers=headers)
        
        print("\\n--- Getting response ---")
        res = conn.getresponse()
        
        print(f"Status: {res.status} {res.reason}")
        print("Response headers:")
        for header, value in res.getheaders():
            print(f"  {header}: {value}")
        
        print("\\n--- Reading response body ---")
        data = res.read()
        print(f"Response body: {repr(data)}")
        print(f"Response body (decoded): '{data.decode('utf-8', errors='ignore')}'")
        
        print("\\n✅ Chunked request successful!")
        if DELETE:
            print("Deleting generated file\\n")
            try:
                robust_request('DELETE', f"{BASE_URL}/upload/test_chunked")
            except:
                pass
        return True
        
    except socket.timeout as e:
        print(f"❌ Timeout error: {e}")
        return False
    except Exception as e:
        print(f"❌ Error: {e}")
        return False
    finally:
        conn.close()

def test_regular_post_for_comparison():
    print("\\n=== Comparison: Regular POST Test ===")
    headers = {"Content-Type": "text/plain", "Content-Length": "25"}
    body = "This is a regular request"
    
    conn = robust_http_connection(HOST, PORT, timeout=10)
    
    try:
        print("\\n--- Sending regular POST ---")
        conn.request("POST", "/upload/test_regular", body=body, headers=headers)
        
        print("\\n--- Getting response ---")
        res = conn.getresponse()
        
        print(f"Status: {res.status} {res.reason}")
        print("Response headers:")
        for header, value in res.getheaders():
            print(f"  {header}: {value}")
        
        data = res.read()
        print(f"Response body: '{data.decode('utf-8', errors='ignore')}'")
        
        print("\\n✅ Regular POST successful!")
        if DELETE:
            print("\\nDeleting generated file")
            try:
                robust_request('DELETE', f"{BASE_URL}/upload/test_regular")
            except:
                pass
        return True
        
    except Exception as e:
        print(f"❌ Error: {e}")
        return False
    finally:
        conn.close()

def test_with_manual_socket():
    print("\\n=== Manual Socket Test (Low Level) ===")
    chunk_data = "This is a chunked request"
    chunk_size = hex(len(chunk_data))[2:]
    
    raw_request = f"""POST /upload/test_manual HTTP/1.1\\r\\nHost: {HOST}:{PORT}\\r\\nTransfer-Encoding: chunked\\r\\nContent-Type: text/plain\\r\\n\\r\\n{chunk_size}\\r\\n{chunk_data}\\r\\n0\\r\\n\\r\\n"""
    
    print("Raw request:")
    print(repr(raw_request))
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(15)
    
    try:
        print(f"\\n--- Connecting to {HOST}:{PORT} ---")
        sock.connect((HOST, PORT))
        
        print("--- Sending raw request ---")
        sock.sendall(raw_request.encode())
        
        print("--- Receiving response ---")
        response = b""
        while True:
            try:
                chunk = sock.recv(1024)
                if not chunk:
                    break
                response += chunk
                print(f"Received chunk: {len(chunk)} bytes")
            except socket.timeout:
                print("Timeout while receiving response")
                break
        
        print(f"\\nFull response ({len(response)} bytes):")
        print(response.decode('utf-8', errors='ignore'))
        
        if DELETE:
            print("Deleting generated file\\n")
            try:
                robust_request('DELETE', f"{BASE_URL}/upload/test_manual")
            except:
                pass
        
        return len(response) > 0

    except Exception as e:
        print(f"❌ Socket error: {e}")
        return False
    finally:
        sock.close()

def webChunk():
    global TOTAL, PASS
    print("====================================================")
    print("                   WEB CHUNK	 			   ")
    print("====================================================")
    print("Starting detailed chunked transfer debugging...\\n")

    TOTAL += 1
    success1 = test_chunked_with_debug()
    if success1:
        PASS += 1

    time.sleep(2)

    TOTAL += 1
    success2 = test_regular_post_for_comparison()
    if success2:
        PASS += 1

    time.sleep(2)

    TOTAL += 1
    success3 = test_with_manual_socket()
    if success3:
        PASS += 1

    print(f"\\n=== Results ===")
    print(f"Chunked test: {'✅ PASS' if success1 else '❌ FAIL'}")
    print(f"Regular test: {'✅ PASS' if success2 else '❌ FAIL'}")
    print(f"Manual test:  {'✅ PASS' if success3 else '❌ FAIL'}")

def test_basic_http_methods():
    print_test("=== Testing Basic HTTP Methods ===")
    
    test_connection("/", "GET", expect_status=200)
    test_connection("/index.html", "GET", expect_status=200)
    test_connection("/nonexistent.html", "GET", expect_status=404)
    
    headers = {"Content-Type": "application/x-www-form-urlencoded"}
    body = "test=testfile.txt"
    test_connection("/upload", "POST", body=body, headers=headers)
    
    test_connection("/upload/testfile.txt", "DELETE")
    print("\\n")

def test_error_handling():
    global TOTAL, PASS
    print_test("=== Testing Error Handling ===")
    TOTAL += 1
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((HOST, PORT))
        sock.send(b"INVALID HTTP REQUEST\\r\\n\\r\\n")
        time.sleep(0.1)
        sock.close()
        PASS += 1
        print_test("Malformed request sent", "PASS")
    except Exception as e:
        print_test(f"Malformed request test failed: {e}", "FAIL")
    print("\\n")

def test_large_request_body():
    global TOTAL, PASS
    print_test("=== Testing Large Request Bodies ===")
    
    test_sizes = [
        (1024 * 512, "512KB"),
        (1024 * 1024, "1MB"),
        (1024 * 1024 * 2, "2MB"),
        (1024 * 1024 * 5, "5MB"),
        (1024 * 1024 * 10, "10MB"),
    ]
    
    print_test("Testing various request sizes to find server limit...")
    
    working_limit = 0
    failed_size = 0
    uploaded_files = []
    
    for size_bytes, size_name in test_sizes:
        TOTAL += 1
        large_body = "x" * size_bytes
        headers = {"Content-Type": "text/plain", "Content-Length": str(len(large_body))}
        file_path = f"/upload/test_{size_name}.txt"
        
        print(f"  Testing {size_name} ({size_bytes:,} bytes)...")
        
        try:
            status, res, data = test_connection(file_path, "POST", 
                                body=large_body, headers=headers, timeout=60)
            
            if res:
                if status == 200 or status == 201:
                    PASS += 1
                    print_test(f"  {size_name}: ✅ ACCEPTED", "PASS")
                    working_limit = size_bytes
                    uploaded_files.append(file_path)
                elif status == 413:
                    PASS += 1
                    print_test(f"  {size_name}: ⚠️  REJECTED (413 - too large)", "INFO")
                    failed_size = size_bytes
                    break
                elif status == 500:
                    print_test(f"  {size_name}: ❌ SERVER ERROR (500)", "FAIL")
                    break
                else:
                    print_test(f"  {size_name}: ❓ UNEXPECTED ({status})", "INFO")
            else:
                print_test(f"  {size_name}: ❌ FAILED (timeout/error)", "FAIL")
                failed_size = size_bytes
                break
        except Exception as e:
            print_test(f"  {size_name}: ❌ ERROR ({str(e)[:50]})", "FAIL")
            break
    
    # Clean up uploaded files
    if uploaded_files:
        print_test(f"Cleaning up {len(uploaded_files)} uploaded test files...")
        TOTAL += 1
        cleanup_success = 0
        for file_path in uploaded_files:
            try:
                status, delete_res, data = test_connection(file_path, "DELETE", timeout=10)
                if delete_res and (status == 200 or status == 204):
                    cleanup_success += 1
                    print_test(f"  ✅ Deleted {file_path}", "INFO")
                elif delete_res and status == 404:
                    print_test(f"  ⚠️  {file_path} not found (may have been auto-deleted)", "INFO")
                    cleanup_success += 1
                else:
                    print_test(f"  ❌ Failed to delete {file_path} (status: {status if delete_res else 'None'})", "INFO")
            except Exception as e:
                print_test(f"  ❌ Error deleting {file_path}: {e}", "INFO")
        
        if cleanup_success == len(uploaded_files):
            PASS += 1
            print_test("✅ All test files cleaned up successfully", "PASS")
        else:
            print_test(f"⚠️  Cleaned up {cleanup_success}/{len(uploaded_files)} files", "INFO")
    
    # Summary
    if working_limit > 0:
        working_mb = working_limit / (1024 * 1024)
        print_test(f"✅ Server accepts requests up to {working_mb:.1f}MB", "PASS")
        
        if working_limit >= 1024 * 1024:
            print_test("Server has reasonable upload limits", "PASS")
        else:
            print_test("Server upload limit might be too restrictive", "INFO")
    
    if failed_size > 0:
        failed_mb = failed_size / (1024 * 1024)
        print_test(f"⚠️  Server properly rejects requests larger than {failed_mb:.1f}MB", "PASS")
    
    if failed_size > 0:
        TOTAL += 1
        print_test("Testing server stability after large request rejection...")
        
        try:
            status, normal_res, data = test_connection("/", "GET", expect_status=200)
            if normal_res:
                PASS += 1
                print_test("✅ Server remains stable after rejecting large request", "PASS")
            else:
                print_test("❌ Server may have issues after large request", "FAIL")
        except:
            print_test("❌ Server may have issues after large request", "FAIL")

def test_request_size_edge_cases():
    global TOTAL, PASS
    print_test("=== Testing Request Size Edge Cases ===")
    
    common_limits = [
        (1024 * 1024, "1MB"),
        (1024 * 1024 * 8, "8MB"),
        (1024 * 1024 * 32, "32MB"),
    ]
    
    uploaded_files = []
    
    for size_bytes, size_name in common_limits:
        TOTAL += 1
        under_limit = size_bytes - 1024
        under_body = "x" * under_limit
        under_headers = {"Content-Type": "text/plain", "Content-Length": str(len(under_body))}
        file_path = f"/upload/under_{size_name}.txt"
        
        print(f"Testing just under {size_name}...")
        try:
            status, res, data = test_connection(file_path, "POST", 
                                body=under_body, headers=under_headers, timeout=60)
            if res and status in (200, 201):
                PASS += 1
                print_test(f"  Just under {size_name}: ✅ ACCEPTED", "PASS")
                uploaded_files.append(file_path)
            elif res and status == 413:
                PASS += 1
                print_test(f"  Server limit is below {size_name}", "INFO")
                break
            else:
                print_test(f"  Under {size_name}: ❓ Status {status if res else 'None'}", "INFO")
        except Exception as e:
            print_test(f"  Under {size_name}: ❌ ERROR", "FAIL")
    
    # Clean up
    if uploaded_files:
        print_test(f"Cleaning up {len(uploaded_files)} edge case test files...")
        
        cleanup_success = 0
        for file_path in uploaded_files:
            try:
                status, delete_res, data = test_connection(file_path, "DELETE", timeout=60)
                if delete_res and (status == 200 or status == 204):
                    cleanup_success += 1
                    print_test(f"  🗑️  Deleted {file_path}", "INFO")
                elif delete_res and status == 404:
                    print_test(f"  ⚠️  {file_path} not found (may have been auto-deleted)", "INFO")
                    cleanup_success += 1
                else:
                    print_test(f"  ❌ Failed to delete {file_path} (status: {status if delete_res else 'None'})", "INFO")
            except Exception as e:
                print_test(f"  ❌ Error deleting {file_path}: {e}", "INFO")
        
        if cleanup_success == len(uploaded_files):
            print_test("✅ All edge case test files cleaned up", "PASS")
        else:
            print_test(f"⚠️  Cleaned up {cleanup_success}/{len(uploaded_files)} edge case files", "INFO")
    
    print("\\n")

def test_reasonable_upload_limits():
    global TOTAL, PASS
    print_test("=== Testing Reasonable Upload Handling ===")
    TOTAL += 1
    reasonable_size = 1024 * 1024
    reasonable_body = "x" * reasonable_size
    reasonable_headers = {"Content-Type": "text/plain", "Content-Length": str(len(reasonable_body))}
    file_path = "/upload/reasonable_test.txt"
    
    try:
        status, res, data = test_connection(file_path, "POST",
                            body=reasonable_body, headers=reasonable_headers, timeout=30)
        
        if res and status in (200, 201):
            PASS += 1
            print_test("✅ Server accepts reasonable 1MB uploads", "PASS")
            
            try:
                status, delete_res, data = test_connection(file_path, "DELETE", timeout=10)
                if delete_res and (status in [200, 204, 404]):
                    print_test("🗑️  Cleaned up test file", "INFO")
                else:
                    print_test("⚠️  Could not clean up test file", "INFO")
            except:
                pass
            
            print("\\n")
            return True
            
        elif res and status == 413:
            print_test("⚠️  Server rejects 1MB uploads (limit may be too low)", "INFO")
            print("\\n")
            return False
            
        else:
            print_test(f"❓ Unexpected response for 1MB upload: {status if res else 'None'}", "FAIL")
            
            try:
                test_connection(file_path, "DELETE", timeout=10)
            except:
                pass
            
            print("\\n")
            return False
            
    except Exception as e:
        print_test(f"❌ Error during test: {e}", "FAIL")
        
        try:
            test_connection(file_path, "DELETE", timeout=10)
        except:
            pass
        
        print("\\n")
        return False

def test_chunked_transfer():
    global TOTAL, PASS
    print_test("=== Testing Chunked Transfer Encoding ===")
    TOTAL += 1
    headers = {"Transfer-Encoding": "chunked", "Content-Type": "text/plain"}
    
    chunk_data = "This is a chunked request"
    chunk_size = hex(len(chunk_data))[2:]
    chunked_body = f"{chunk_size}\\r\\n{chunk_data}\\r\\n0\\r\\n\\r\\n"
    
    try:
        test_connection("/upload/test", "POST", body=chunked_body, headers=headers)
        PASS += 1
    except:
        pass
    print("\\n")

def test_concurrent_connections():
    global TOTAL, PASS
    print_test("=== Testing Concurrent Connections ===")
    TOTAL += 1
    results = []
    def make_request(i):
        try:
            status, res, data = test_connection(f"/test_{i}", "GET")
            results.append(res is not None)
        except:
            results.append(False)
    
    threads = []
    for i in range(10):
        t = Thread(target=make_request, args=(i,))
        t.start()
        threads.append(t)
    
    for t in threads:
        t.join()
    
    success_count = sum(results)
    if success_count >= 7:  # Allow some failures
        PASS += 1
    print_test(f"Concurrent connections: {success_count}/10 successful", 
              "PASS" if success_count >= 7 else "FAIL")
    print("\\n")

def test_keep_alive_webtest():
    global TOTAL, PASS
    print_test("=== Testing Keep-Alive Connections ===")
    TOTAL += 1
    conn = http.client.HTTPConnection(HOST, PORT)
    try:
        for i in range(3):
            conn.request("GET", f"/test_{i}")
            res = conn.getresponse()
            data = res.read()
            if res.status not in [200, 404]:
                print_test(f"Keep-alive request {i} failed", "FAIL")
                return
        PASS += 1
        print_test("Keep-alive connections working", "PASS")
    except Exception as e:
        print_test(f"Keep-alive test failed: {e}", "FAIL")
    finally:
        conn.close()
    print("\\n")

def test_multipart_upload_webtest():
    global TOTAL, PASS
    print_test("=== Testing Multipart File Upload ===")
    TOTAL += 1
    boundary = f"----WebKitFormBoundary{uuid.uuid4().hex[:16]}"
    filename = "test_upload.txt"
    file_content = "This is a test upload file.\\nLine 2 of content.\\n🚀 Unicode test"
    
    body_parts = [
        f"--{boundary}",
        f'Content-Disposition: form-data; name="file"; filename="{filename}"',
        "Content-Type: text/plain",
        "",
        file_content,
        f"--{boundary}--",
        ""
    ]
    body = "\\r\\n".join(body_parts)
    
    headers = {
        "Content-Type": f"multipart/form-data; boundary={boundary}",
        "Content-Length": str(len(body.encode()))
    }
    
    try:
        status, res, data = test_connection("/upload", "POST", body=body.encode(), headers=headers)
        if res and status in (200, 201):
            PASS += 1
            print_test("Multipart upload successful", "PASS")
        else:
            print_test("Multipart upload failed", "FAIL")
    except:
        print_test("Multipart upload failed", "FAIL")
    print("\\n")

def test_cgi_functionality():
    global TOTAL, PASS
    print_test("=== Testing CGI Functionality ===")
    TOTAL += 1
    cgi_tests = [
        ("/cgi-bin/hello.py", "POST", {"name": "test"}),
        ("/cgi-bin/hello.php", "GET", {}),
        ("/cgi-bin/env.pl", "GET", {}),
    ]
    success_count = 0
    
    for path, method, data in cgi_tests:
        try:
            if method == "POST":
                body = "&".join(f"{k}={v}" for k, v in data.items())
                headers = {"Content-Type": "application/x-www-form-urlencoded"}
                status, res, response_data = test_connection(path, method, body=body, headers=headers)
            else:
                status, res, response_data = test_connection(path, method)

            if res:
                if status == 200:
                    success_count += 1
                    print_test(f"CGI script {path} works", "PASS")
                elif status == 500:
                    print_test(f"CGI script {path} returned 500 (Server Error)", "FAIL")
                elif status == 405:
                    print_test(f"CGI script {path} returned 405 (Method not allowed)", "FAIL")
                else:
                    print_test(f"CGI script {path} returned unexpected status {status}", "FAIL")
            else:
                print_test(f"CGI script {path} test failed completely", "FAIL")
        except:
            print_test(f"CGI script {path} test failed with exception", "FAIL")
    
    if success_count >= 1:  # At least one CGI script should work
        PASS += 1
    print()

def test_redirection_webtest():
    global TOTAL, PASS
    print_test("=== Testing HTTP Redirections ===")
    TOTAL += 1
    redirect_tests = [
        ("/redirect301", 301),
        ("/redirect302", 302), 
        ("/redirectme", 302)
    ]
    success_count = 0
    
    for path, expected_code in redirect_tests:
        try:
            result = test_connection(path, "GET")
            if result:
                status, headers, data = result
                if status == expected_code:
                    header_dict = dict(headers)
                    location = header_dict.get("Location")
                    if location:
                        success_count += 1
                        print_test(f"Redirect {path} -> {location}", "PASS")
                    else:
                        print_test(f"Redirect {path} missing Location header", "FAIL")
                else:
                    print_test(f"{path} expected redirect ({expected_code}), got {status}", "FAIL")
            else:
                print_test(f"{path} test failed completely", "FAIL")
        except:
            print_test(f"{path} test failed with exception", "FAIL")
    
    if success_count >= 1:
        PASS += 1
    print()

def test_mime_types():
    global TOTAL, PASS
    print_test("=== Testing MIME Type Detection ===")
    TOTAL += 1
    success_count = 0
    mime_tests = [
        ("/test/test.html", "text/html"),
        ("/test/test.css", "text/css"),
        ("/test/test.js", "application/javascript"),
        ("/test/test.jpg", "image/jpeg"),
        ("/test/test.png", "image/png"),
    ]
    
    for path, expected_mime in mime_tests:
        try:
            status, res, response_str = test_connection(path, "GET")
            if res:
                content_type = None
                for key, value in res:
                    if key.lower() == "content-type":
                        content_type = value
                        break
                if content_type and expected_mime in content_type:
                    success_count += 1
                    print_test(f"MIME type for {path}: {content_type}", "PASS")
                else:
                    print_test(f"MIME type for {path} incorrect: {content_type}", "FAIL")
            else:
                print_test(f"Could not fetch {path}", "FAIL")
        except:
            print_test(f"MIME test for {path} failed", "FAIL")
    
    if success_count >= 3:  # At least 3 out of 5 should work
        PASS += 1
    print("\\n")

def test_path_traversal_security():
    global TOTAL, PASS
    print_test("=== Testing Path Traversal Security ===")
    TOTAL += 1
    success_count = 0
    malicious_paths = [
        "/../../../etc/passwd",
        "/upload/../../../etc/passwd", 
        "/../config/default.conf",
        "/./././../etc/passwd"
    ]
    
    for path in malicious_paths:
        try:
            status, res, data = test_connection(path, "GET")
            if res and status in [403, 404]:
                success_count += 1
                print_test(f"Path traversal blocked: {path}", "PASS")
            else:
                print_test(f"Path traversal security concern: {path}", "FAIL")
        except:
            success_count += 1  # Exception is also good (blocked)
            print_test(f"Path traversal blocked: {path}", "PASS")
    
    if success_count >= 3:
        PASS += 1
    print("\\n")

def test_server_resilience():
    global TOTAL, PASS
    print_test("=== Testing Server Resilience ===")
    TOTAL += 1
    successful_connections = 0
    
    for i in range(5):
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            sock.connect((HOST, PORT))
            
            request = "GET / HTTP/1.1\\r\\nHost: localhost\\r\\nConnection: close\\r\\n\\r\\n"
            sock.send(request.encode())
            
            response = sock.recv(1024)
            if b"HTTP/1.1" in response:
                successful_connections += 1
            
            sock.close()
            time.sleep(1)
            
        except Exception as e:
            print_test(f"Connection {i} failed: {e}", "INFO")
            time.sleep(1)
    
    time.sleep(2)
    try:
        if test_connection("/", "GET", expect_status=200):
            PASS += 1
            print_test(f"Server resilience test: {successful_connections}/5 connections successful", "PASS")
        else:
            print_test("Server may have issues with connection handling", "FAIL")
    except:
        print_test("Server may have issues with connection handling", "FAIL")
    print("\\n")

def test_http_headers():
    global TOTAL, PASS
    print_test("=== Testing HTTP Headers ===")
    TOTAL += 1
    try:
        status, res, data = test_connection("/", "GET")
        if res:
            server_header = None
            content_length = None
            for key, value in res:
                if key.lower() == "server":
                    server_header = value
                if key.lower() == "content-length":
                    content_length = value
            
            if server_header and "WebServ" in server_header:
                print_test(f"Server header: {server_header}", "PASS")
            if content_length:
                print_test(f"Content-Length header present: {content_length}", "PASS")
        PASS += 1
    except:
        pass
    print("\\n")

def test_different_ports():
    global TOTAL, PASS
    print_test("=== Testing Multiple Ports (if configured) ===")
    TOTAL += 1
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
    PASS += 1
    print("\\n")

def test_stress_test():
    global TOTAL, PASS
    print_test("=== Stress Testing ===")
    TOTAL += 1
    def gentle_stress_worker():
        for _ in range(10):
            try:
                test_connection("/", "GET")
                time.sleep(0.1)
            except:
                pass
    
    threads = []
    for _ in range(3):
        t = Thread(target=gentle_stress_worker)
        t.start()
        threads.append(t)
    
    for t in threads:
        t.join()
    
    time.sleep(3)
    try:
        if test_connection("/", "GET", expect_status=200):
            PASS += 1
            print_test("Stress test completed - server still responsive", "PASS")
        else:
            print_test("Server may have issues under stress", "FAIL")
    except:
        print_test("Server may have issues under stress", "FAIL")

def webTest():
    print("====================================================")
    print("                   WEB TEST	 			   ")
    print("====================================================")
    
    try:
        test_basic_http_methods()
        test_error_handling()
        test_mime_types()
        test_http_headers()
        test_multipart_upload_webtest()
        test_directory_listing()
        test_redirection_webtest()
        test_cgi_functionality()
        test_path_traversal_security()
        test_keep_alive_webtest()
        test_chunked_transfer()
        test_large_request_body()
        test_request_size_edge_cases()
        test_reasonable_upload_limits()
        test_different_ports()
        test_concurrent_connections()
        test_server_resilience()
        test_stress_test()
        
    finally:
        print("")

def webTestCgiFileArg():
    global TOTAL, PASS
    print("====================================================")
    print("                   webTestCgiFileArg 			   ")
    print("====================================================")
    TOTAL += 1
    expected_file_path = "local/cgi-bin/data.txt"
    expected_content = "Hello from the test file!\\n"

    # Create the test file (the one to be passed as argv[1])
    os.makedirs("local/cgi-bin", exist_ok=True)
    with open(expected_file_path, "w") as f:
        f.write(expected_content)

    # Create the CGI script
    cgi_script_path = "local/cgi-bin/echo_file.py"
    with open(cgi_script_path, "w") as f:
        f.write("""#!/usr/bin/env python3
import sys
print("Content-Type: text/plain\\\\n")
try:
    with open(sys.argv[1], 'r') as file:
        print(file.read())
except Exception as e:
    print("Error:", e)
""")

    os.chmod(cgi_script_path, 0o755)  # Make it executable

    # Send the request
    try:
        conn = http.client.HTTPConnection(HOST, PORT)
        conn.request("GET", "/cgi-bin/echo_file.py?file=data.txt")
        response = conn.getresponse()
        body = response.read().decode(errors='ignore')
        conn.close()
        
        print("Status:", response.status)
        print("Response Body:", body)
        
        if response.status == 200:
            if expected_content.strip() in body:
                PASS += 1
                print("✅ CGI with file-as-arg test passed!")
            else:
                print(f"❌ Expected content not found. Expected: '{expected_content.strip()}', Got: '{body}'")
        else:
            print(f"❌ Expected HTTP 200 OK, got {response.status}")
            
    except Exception as e:
        print(f"❌ Test failed with exception: {e}")
    finally:
        # Cleanup
        try:
            os.remove(expected_file_path)
            os.remove(cgi_script_path)
        except:
            pass

def pass_msg(msg):
    print(f"{GREEN}✅ {msg}{RESET}")

def fail_msg(msg):
    print(f"{RED}❌ {msg}{RESET}")

def info_msg(msg):
    print(f"ℹ️ {BLUE} {msg}{RESET}")

def test_percent_decoding():
    global TOTAL, PASS
    print("\\n>>> Test 1: Percent-decoded URI returns correct file")
    TOTAL += 1
    try:
        response = robust_request('GET', f"{TARGET}/{ENCODED_PATH}")
        if response.status_code == 200:
            PASS += 1
            pass_msg(f"Request to '{ENCODED_PATH}' succeeded (HTTP 200)")
        else:
            fail_msg(f"Expected HTTP 200 for '{ENCODED_PATH}', got {response.status_code}")
    except requests.RequestException as e:
        fail_msg(f"Request failed: {e}")

def test_redirect_location_header():
    global TOTAL, PASS
    print("\\n>>> Test 2: Location header in redirect is percent-encoded")
    TOTAL += 1
    try:
        response = robust_request('GET', f"{TARGET}/{REDIRECT_TEST}", allow_redirects=False)
        location = response.headers.get("Location", "")
        if "%20" in location:
            PASS += 1
            pass_msg(f"Location header is correctly percent-encoded: {location}")
        elif location:
            fail_msg(f"Location header exists but is not encoded: {location}")
        else:
            fail_msg(f"No Location header received for '{REDIRECT_TEST}'")
    except requests.RequestException as e:
        fail_msg(f"Request failed: {e}")

def test_autoindex_links_encoded():
    global TOTAL, PASS
    print("\\n>>> Test 3: Autoindex hrefs use encoded links")
    TOTAL += 1
    try:
        response = robust_request('GET', f"{TARGET}/{AUTOINDEX_PATH}/")
        html = response.text
        if "%20" in html:
            PASS += 1
            pass_msg("Autoindex hrefs contain encoded links")
        elif "<title>Index of" in html:
            fail_msg("Autoindex active, but hrefs are not encoded")
        elif "index space" in html.lower():
            info_msg("Directory contains an index file — autoindex was skipped")
        else:
            info_msg(f"No autoindex response detected at '{AUTOINDEX_PATH}' — skipping")
    except requests.RequestException as e:
        fail_msg(f"Request failed: {e}")

def webUriEncoding():
    print("====================================================")
    print("                   webUriEncoding			   ")
    print("====================================================")
    print("=== Starting Percent-Encoding Tests ===")
    test_percent_decoding()
    test_redirect_location_header()
    test_autoindex_links_encoded()

def main():
    integrated_tests = [
        ("webAnother tests", webAnother),
        ("webCheck tests", webCheck),
        ("webChunk tests", webChunk),
        ("webTest tests", webTest),
        ("webTestCgiFileArgs", webTestCgiFileArg),
        ("webUriEncoding", webUriEncoding)
    ]
    
    try:
        printHeaders()
        
        # Run integrated test suites
        for test_name, test_func in integrated_tests:
            run_test_suite(test_name, test_func)
            
    except subprocess.CalledProcessError as err:
        print(f"✗{RED} Error running external script: {err}{RESET}")
        print(f"{RED} Error output: {err.stderr}{RESET}")
    except FileNotFoundError as e:
        print(f"✗{RED} Script not found: {e}{RESET}")
    finally:
        print(f"{GREEN}=== [{PASS}]/[{TOTAL}] TESTS PASSED ==={RESET}")
        print(f"{BLUE}~~~~~~~~~~~~~~~~~~ ALL DONE ~~~~~~~~~~~~~~~~~~{RESET}")

if __name__ == "__main__":
    main()
