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

# === Colors ===
RED = "\033[31"
BLUE = "\33[34"
GREEN = "\33[32"
RESET = "\033[0"

# === Config ===
HOST = "localhost"
PORT = 8080
BASE_URL = f"http://{HOST}:{PORT}"
DELETE = True

# === Colour Message ===
def success(msg): print(f"GREEN✅ {msg}RESET")
def fail(msg): print(f"RED❌ {msg}RESET")
def header(msg): print(f"\n===> {msg}")

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
def make_temp_file(content="this is a test upload"):
    f = tempfile.NamedTemporaryFile(delete=False, mode='w')
    f.write(content)
    f.close()
    return f.name

def make_big_file(size_mb=20):
    path = tempfile.NamedTemporaryFile(delete=False).name
    with open(path, "wb") as f:
        f.write(b"\0" * (size_mb * 1024 * 1024))
    return path

def test_get_root(): 
    print("Basic GET Request")
    requests.get(BASE_URL).raise_for_status()

def test_get_index():
    print("Static File Request (/index.html)")
    requests.get(f"{BASE_URL}/index.html").raise_for_status()

def test_404(): 
    print("404 Not Found")
    print("Static File Request (/index.html)")
    assert requests.get(f"{BASE_URL}/nope.html").status_code == 404

def test_403():
    print("403 Forbidden (no permission dir")
    url = f"{BASE_URL}/forbidden_dir/"
    response = requests.get(url)
    assert response.status_code == 403, f"Expected 403, got {response.status_code}"

def test_autoindex():
    print("Autoindex Directory")
    r = requests.get(f"{BASE_URL}/listing/")
    if "<title>Index of" not in r.text:
        raise Exception("Missing autoindex title")
    
def test_post_form():
    print("POST Form Data")
    r = requests.post(f"{BASE_URL}/form-handler", data={'foo': 'bar'})
    r.raise_for_status()
    assert "foo" in r.text.lower() or "bar" in r.text.lower() or "success" in r.text.lower()
    
def test_upload_text(temp_file):
    print("File Upload (text/plain)")
    filename = os.path.basename(temp_file)
    with open(temp_file, 'rb') as f:
        r = requests.post(f"{BASE_URL}/upload", files={'file': (filename, f)})
        r.raise_for_status()
    r_check = requests.get(f"{BASE_URL}/upload/{filename}")
    assert r_check.status_code == 200
    assert "this is a test upload" in r_check.text

def test_multipart_upload(temp_file):
    filename = os.path.basename(temp_file)
    with open(temp_file, 'rb') as f:
        r = requests.post(f"{BASE_URL}/upload", files={'file': (filename, f)})
        r.raise_for_status()
    r_check = requests.get(f"{BASE_URL}/upload/{filename}")
    assert r_check.status_code == 200
    assert "this is a test upload" in r_check.text

def test_delete_uploaded(temp_file):
    print("DELETE Uploaded File")
    filename = os.path.basename(temp_file)
    requests.delete(f"{BASE_URL}/upload/{filename}").raise_for_status()
    r = requests.get(f"{BASE_URL}/upload/{filename}")
    assert r.status_code == 404

def test_redirect():
    print("HTTP Redirect (301/302)")
    r = requests.get(f"{BASE_URL}/redirect", allow_redirects=False)
    assert r.status_code in (301, 302)
    assert "Location" in r.headers
    r_follow = requests.get(f"{BASE_URL}/redirect", allow_redirects=True)
    r_follow.raise_for_status()

def test_virtual_host():
    print("Virtual Host")
    r = requests.get(BASE_URL, headers={"Host": "myvirtualhost"})
    r.raise_for_status()

def test_cgi_get():
    print("CGI Execution (GET)")
    r = requests.get(f"{BASE_URL}/cgi-bin/hello.py?param=value")
    r.raise_for_status()
    assert "param" in r.text.lower() or "hello" in r.text.lower()

def test_cgi_post():
    print("CGI Execution (POST)")
    r = requests.post(f"{BASE_URL}/cgi-bin/hello.py", data={"input": "abc"})
    r.raise_for_status()
    assert "abc" in r.text.lower() or "input" in r.text.lower()

def test_chunked_encoding():
    print("Chunked Transfer Encoding")
    conn = http.client.HTTPConnection(HOST, PORT, timeout=15)
    conn.putrequest("POST", "/upload/test_chunked")
    conn.putheader("Transfer-Encoding", "chunked")
    conn.putheader("Content-Type", "text/plain")
    conn.endheaders()
    
    chunk_data = "this is a test upload"
    chunk_size = hex(len(chunk_data))[2:]
    conn.send((chunk_size + "\r\n").encode())
    conn.send(chunk_data.encode())            
    conn.send(b"\r\n")
    conn.send(b"0\r\n\r\n")
    
    response = conn.getresponse()
    smth = response.read()
    print(f"Chunk encoded: {smth}")
    body = smth.decode(errors='ignore')
    print(f"Response status: {response.status}")
    print(f"Response body: {body}")
    if response.status != 200:
        raise Exception(f"Expected 200 OK, got {response.status}")
    r_check = requests.get(f"{BASE_URL}/upload/test_chunked")
    assert r_check.status_code == 200
    assert "this is a test upload" in r_check.text
    if DELETE == True:
        conn.request("DELETE", "/upload/test_chunked")
    conn.close()

def test_put_not_allowed():
    print("Method Not Allowed (PUT)")
    r = requests.put(BASE_URL + "/")
    if r.status_code != 405:
        raise Exception(f"Expected 405 Method Not Allowed, got {r.status_code}")

def test_keep_alive():
    print("Keep-Alive Header (Connection reuse)")
    r = requests.get(BASE_URL, headers={"Connection": "keep-alive"})
    r.raise_for_status()

def test_http_0_9():
    print("Unsupported HTTP Version")
    raw = b"GET / HTTP/0.9\r\nHost: localhost\r\n\r\n"
    result = subprocess.run(["nc", HOST, str(PORT)], input=raw, stdout=subprocess.PIPE)
    if b"HTTP/1.1 400 Bad Request" not in result.stdout:
        raise Exception("Expected HTTP 400 for HTTP/0.9")

def test_missing_host():
    print("Missing Host Header (HTTP/1.1)")
    raw = b"GET / HTTP/1.1\r\n\r\n"
    result = subprocess.run(["nc", HOST, str(PORT)], input=raw, stdout=subprocess.PIPE)
    if b"HTTP/1.1 400 Bad Request" not in result.stdout:
        raise Exception("Expected 400 for missing Host header")

def test_concurrent_gets():
    print("Multiple Concurrent GET Requests (x10)")
    def get(): return requests.get(BASE_URL).status_code
    with concurrent.futures.ThreadPoolExecutor(max_workers=10) as executor:
        results = list(executor.map(lambda _: get(), range(10)))
    if any(code != 200 for code in results):
        raise Exception(f"Some requests failed: {results}")

def test_invalid_method():
    print("Invalid HTTP Method (via netcat workaround)")
    raw = b"FOO / HTTP/1.1\r\nHost: localhost\r\n\r\n"
    result = subprocess.run(["nc", HOST, str(PORT)], input=raw, stdout=subprocess.PIPE)
    if b"HTTP/1.1 400 Bad Request" not in result.stdout:
        raise Exception("Expected 400 for invalid method")

def test_large_body_rejected(big_file):
    print("Large Request Body (20MB)")
    with open(big_file, "rb") as f:
        r = requests.post(f"{BASE_URL}/upload", data=f)
        assert r.status_code >= 400
        assert r.status_code in (413, 500, 400), f"Unexpected status: {r.status_code}"
    
def webAnother():
    print(f"Webserv Testing Started at {BASE_URL}")
    print("=====================================")

    temp_file = make_temp_file()
    big_file = make_big_file()
    FORBIDDEN_DIR = tempfile.mkdtemp()
    os.chmod(FORBIDDEN_DIR, 0o000)
    WEB_ROOT = "./local/"
    linked_dir = os.path.join(WEB_ROOT, "forbidden_dir")
    if os.path.islink(linked_dir) or os.path.exists(linked_dir):
        os.remove(linked_dir)
    os.symlink(FORBIDDEN_DIR, linked_dir)

    test_get_root()
    test_get_index()
    test_404()
    test_403()
    test_autoindex()
    test_post_form()
    test_upload_text(temp_file)
    test_multipart_upload(temp_file)
    test_delete_uploaded(temp_file)
    test_redirect()
    test_virtual_host()
    test_cgi_get()
    test_cgi_post()
    test_chunked_encoding()
    test_put_not_allowed()
    test_keep_alive()
    test_http_0_9()
    test_missing_host()
    test_concurrent_gets()
    test_invalid_method()
    test_large_body_rejected(big_file)

    os.remove(temp_file)
    os.remove(big_file)
    os.chmod(FORBIDDEN_DIR, 0o700)
    os.remove(linked_dir)
    os.rmdir(FORBIDDEN_DIR)

def main():
    try:
        printHeaders()
        webAnother()
        print("====================================================")
        print("                   WEB ANOTHER	 			   ")
        print("====================================================")
		
    finally:
        print("All done!")

if __name__ == "__main__":
    main()