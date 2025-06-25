#!/usr/bin/env python3

import os
import socket
import http.client
import requests
import subprocess
import tempfile
import concurrent.futures
from colorama import Fore, Style

# === CONFIG ===
HOST = "localhost"
PORT = 8080
BASE_URL = f"http://{HOST}:{PORT}"
PASS_COUNT = 0
TOTAL_COUNT = 0

# === COLORS ===
def success(msg): print(f"{Fore.GREEN}✅ {msg}{Style.RESET_ALL}")
def fail(msg): print(f"{Fore.RED}❌ {msg}{Style.RESET_ALL}")
def header(msg): print(f"\n===> {msg}")

# === UTILITIES ===
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
        print(e)

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

# === TEST FUNCTIONS ===
def test_get_root(): requests.get(BASE_URL).raise_for_status()
def test_get_index(): requests.get(f"{BASE_URL}/index.html").raise_for_status()
def test_404(): assert requests.get(f"{BASE_URL}/nope.html").status_code == 404
def test_403():
    url = f"{BASE_URL}/forbidden_dir/"
    response = requests.get(url)
    assert response.status_code == 403, f"Expected 403, got {response.status_code}"

def test_autoindex():
    r = requests.get(f"{BASE_URL}/listing/")
    if "<title>Index of" not in r.text:
        raise Exception("Missing autoindex title")

def test_post_form():
    r = requests.post(f"{BASE_URL}/form-handler", data={'foo': 'bar'})
    r.raise_for_status()
    assert "foo" in r.text.lower() or "bar" in r.text.lower() or "success" in r.text.lower()

def test_upload_text():
    filename = os.path.basename(temp_file)
    with open(temp_file, 'rb') as f:
        r = requests.post(f"{BASE_URL}/upload", files={'file': (filename, f)})
        r.raise_for_status()
    r_check = requests.get(f"{BASE_URL}/upload/{filename}")
    assert r_check.status_code == 200
    assert "this is a test upload" in r_check.text

def test_multipart_upload():
    filename = os.path.basename(temp_file)
    with open(temp_file, 'rb') as f:
        r = requests.post(f"{BASE_URL}/upload", files={'file': (filename, f)})
        r.raise_for_status()
    r_check = requests.get(f"{BASE_URL}/upload/{filename}")
    assert r_check.status_code == 200
    assert "this is a test upload" in r_check.text

def test_delete_uploaded():
    filename = os.path.basename(temp_file)
    requests.delete(f"{BASE_URL}/upload/{filename}").raise_for_status()
    r = requests.get(f"{BASE_URL}/upload/{filename}")
    assert r.status_code == 404

def test_redirect():
    r = requests.get(f"{BASE_URL}/redirectme", allow_redirects=False)
    assert r.status_code in (301, 302)
    assert "Location" in r.headers
    r_follow = requests.get(f"{BASE_URL}/redirectme", allow_redirects=True)
    r_follow.raise_for_status()

def test_virtual_host():
    r = requests.get(BASE_URL, headers={"Host": "myvirtualhost"})
    r.raise_for_status()

def test_cgi_get():
    r = requests.get(f"{BASE_URL}/cgi-bin/hello.py?param=value")
    r.raise_for_status()
    assert "param" in r.text.lower() or "hello" in r.text.lower()

def test_cgi_post():
    r = requests.post(f"{BASE_URL}/cgi-bin/hello.py", data={"input": "abc"})
    r.raise_for_status()
    assert "abc" in r.text.lower() or "input" in r.text.lower()

def test_chunked_encoding():
    filename = "chunked_test.txt"
    temp_path = make_temp_file("this is a test upload")
    with open(temp_path, "rb") as f:
        content = f.read()

    conn = http.client.HTTPConnection(HOST, PORT, timeout=10)
    conn.putrequest("POST", f"/upload/{filename}")
    conn.putheader("Transfer-Encoding", "chunked")
    conn.putheader("Content-Type", "application/octet-stream")
    conn.putheader("Host", f"{HOST}:{PORT}")
    conn.putheader("Connection", "close")
    conn.endheaders()

    chunk_size = 10
    offset = 0
    while offset < len(content):
        chunk = content[offset : offset + chunk_size]
        offset += chunk_size

        conn.send(f"{len(chunk):X}\r\n".encode())
        conn.send(chunk + b"\r\n")

    conn.send(b"0\r\n\r\n")

    response = conn.getresponse()
    status = response.status
    body = response.read().decode(errors="ignore")

    print(f"Response status: {status}")
    print("Response body:")
    print(body)

    if status != 200:
        raise Exception(f"Expected 200 OK, got {status}")

    r_check = requests.get(f"{BASE_URL}/upload/{filename}")
    assert r_check.status_code == 200
    assert "this is a test upload" in r_check.text

    conn.close()
    os.remove(temp_path)

def test_put_not_allowed():
    r = requests.put(BASE_URL + "/")
    if r.status_code != 405:
        raise Exception(f"Expected 405 Method Not Allowed, got {r.status_code}")

def test_keep_alive():
    r = requests.get(BASE_URL, headers={"Connection": "keep-alive"})
    r.raise_for_status()

def test_http_0_9():
    raw = b"GET / HTTP/0.9\r\nHost: localhost\r\n\r\n"
    result = subprocess.run(["nc", HOST, str(PORT)], input=raw, stdout=subprocess.PIPE)
    if b"HTTP/1.1 400 Bad Request" not in result.stdout:
        raise Exception("Expected HTTP 400 for HTTP/0.9")

def test_missing_host():
    raw = b"GET / HTTP/1.1\r\n\r\n"
    result = subprocess.run(["nc", HOST, str(PORT)], input=raw, stdout=subprocess.PIPE)
    if b"HTTP/1.1 400 Bad Request" not in result.stdout:
        raise Exception("Expected 400 for missing Host header")

def test_concurrent_gets():
    def get(): return requests.get(BASE_URL).status_code
    with concurrent.futures.ThreadPoolExecutor(max_workers=10) as executor:
        results = list(executor.map(lambda _: get(), range(10)))
    if any(code != 200 for code in results):
        raise Exception(f"Some requests failed: {results}")

def test_invalid_method():
    raw = b"FOO / HTTP/1.1\r\nHost: localhost\r\n\r\n"
    result = subprocess.run(["nc", HOST, str(PORT)], input=raw, stdout=subprocess.PIPE)
    if b"HTTP/1.1 400 Bad Request" not in result.stdout:
        raise Exception("Expected 400 for invalid method")

def test_large_body_rejected():
    with open(big_file, "rb") as f:
        r = requests.post(f"{BASE_URL}/upload", data=f)
        assert r.status_code >= 400
        assert r.status_code in (413, 500, 400), f"Unexpected status: {r.status_code}"

# === MAIN ===
if __name__ == "__main__":
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

    tests = [
        ("Basic GET Request", test_get_root),
        ("Static File Request (/index.html)", test_get_index),
        ("404 Not Found", test_404),
        ("403 Forbidden (no permission dir)", test_403),
        ("Autoindex Directory", test_autoindex),
        ("POST Form Data", test_post_form),
        ("File Upload (text/plain)", test_upload_text),
        ("Multipart Form Upload", test_multipart_upload),
        ("DELETE Uploaded File", test_delete_uploaded),
        ("HTTP Redirect (301/302)", test_redirect),
        ("Virtual Host", test_virtual_host),
        ("CGI Execution (GET)", test_cgi_get),
        ("CGI Execution (POST)", test_cgi_post),
        # ("Chunked Transfer Encoding", test_chunked_encoding),//TODO: this fucker
        ("Method Not Allowed (PUT)", test_put_not_allowed),
        ("Keep-Alive Header (Connection reuse)", test_keep_alive),
        ("Unsupported HTTP Version", test_http_0_9),
        ("Missing Host Header (HTTP/1.1)", test_missing_host),
        ("Multiple Concurrent GET Requests (x10)", test_concurrent_gets),
        ("Invalid HTTP Method (via netcat workaround)", test_invalid_method),
        ("Large Request Body (20MB)", test_large_body_rejected)
    ]

    for description, test_func in tests:
        run_test(description, test_func)

    os.remove(temp_file)
    os.remove(big_file)
    os.chmod(FORBIDDEN_DIR, 0o700)
    os.remove(linked_dir)
    os.rmdir(FORBIDDEN_DIR)

    print("\n=====================================")
    print(f"✅ {Fore.GREEN}{PASS_COUNT}{Style.RESET_ALL} / {TOTAL_COUNT} tests passed")
    print("=====================================")
