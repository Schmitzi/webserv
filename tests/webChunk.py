#!/usr/bin/env python3

import http.client
import socket
import time
# from colorama import Fore, Style

HOST = "127.0.0.1"
PORT = 8080

def test_chunked_with_debug():
    print("=== Debug Chunked Transfer Test ===")
    
    # Test chunked request with detailed debugging
    headers = {"Transfer-Encoding": "chunked", "Content-Type": "text/plain"}
    
    # Manual chunked body
    chunk_data = "This is a chunked request"
    chunk_size = hex(len(chunk_data))[2:]
    chunked_body = f"{chunk_size}\r\n{chunk_data}\r\n0\r\n\r\n"
    
    print(f"Chunk size hex: {chunk_size}")
    print(f"Chunk data: '{chunk_data}'")
    print(f"Full chunked body: {repr(chunked_body)}")
    
    # Create connection with debugging
    conn = http.client.HTTPConnection(HOST, PORT, timeout=15)
    conn.set_debuglevel(1)  # Enable HTTP debugging
    
    try:
        print("\n--- Sending request ---")
        conn.request("POST", "/upload/test_chunked", body=chunked_body, headers=headers)
        
        print("\n--- Getting response ---")
        res = conn.getresponse()
        
        print(f"Status: {res.status} {res.reason}")
        print("Response headers:")
        for header, value in res.getheaders():
            print(f"  {header}: {value}")
        
        print("\n--- Reading response body ---")
        data = res.read()
        print(f"Response body: {repr(data)}")
        print(f"Response body (decoded): '{data.decode('utf-8', errors='ignore')}'")
        
        print("\n✅ Chunked request successful!")
        # print("Deleting generated file\n")
        # conn.request("DELETE", "/upload/test_chunked")
        return True
        
    except socket.timeout as e:
        print(f"❌ Timeout error: {e}")
        # print("Deleting generated file\n")
        # conn.request("DELETE", "/upload/test_chunked")
        return False
    except Exception as e:
        print(f"❌ Error: {e}")
        # print("Deleting generated file\n")
        # conn.request("DELETE", "/upload/test_chunked")
        return False
    finally:
        conn.close()

def test_regular_post_for_comparison():
    print("\n=== Comparison: Regular POST Test ===")
    
    headers = {"Content-Type": "text/plain", "Content-Length": "25"}
    body = "This is a regular request"
    
    conn = http.client.HTTPConnection(HOST, PORT, timeout=10)
    conn.set_debuglevel(1)
    
    try:
        print("\n--- Sending regular POST ---")
        conn.request("POST", "/upload/test_regular", body=body, headers=headers)
        
        print("\n--- Getting response ---")
        res = conn.getresponse()
        
        print(f"Status: {res.status} {res.reason}")
        print("Response headers:")
        for header, value in res.getheaders():
            print(f"  {header}: {value}")
        
        data = res.read()
        print(f"Response body: '{data.decode('utf-8', errors='ignore')}'")
        
        print("\n✅ Regular POST successful!")
        # print("\nDeleting generated file")
        # conn.request("DELETE", "/upload/test_regular");
        return True
        
    except Exception as e:
        print(f"❌ Error: {e}")
        return False
    finally:
        conn.close()

def test_with_manual_socket():
    print("\n=== Manual Socket Test (Low Level) ===")
    
    # Raw HTTP request
    chunk_data = "This is a chunked request"
    chunk_size = hex(len(chunk_data))[2:]
    
    raw_request = f"""POST /upload/test_manual HTTP/1.1\r\nHost: {HOST}:{PORT}\r\nTransfer-Encoding: chunked\r\nContent-Type: text/plain\r\n\r\n{chunk_size}\r\n{chunk_data}\r\n0\r\n\r\n"""
    
    print("Raw request:")
    print(repr(raw_request))
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(15)
    
    try:
        print(f"\n--- Connecting to {HOST}:{PORT} ---")
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
        
        print(f"\nFull response ({len(response)} bytes):")
        print(response.decode('utf-8', errors='ignore'))
        # print("Deleting generated file\n")
        # delete_request = "DELETE /upload/test_manual HTTP/1.1"
        # sock.send(delete_request.encode());
        
        return len(response) > 0
        
    except Exception as e:
        print(f"❌ Socket error: {e}")
        return False
    finally:
        sock.close()

if __name__ == "__main__":
    print("Starting detailed chunked transfer debugging...\n")

    # Test 1: Chunked request
    success1 = test_chunked_with_debug()

    time.sleep(2)

    # Test 2: Regular POST for comparison  
    success2 = test_regular_post_for_comparison()

    time.sleep(2)

    # Test 3: Manual socket test
    success3 = test_with_manual_socket()

    print(f"\n=== Results ===")
    print(f"Chunked test: {'✅ PASS' if success1 else '❌ FAIL'}")
    # print(f"Regular test: {'✅ PASS' if success2 else '❌ FAIL'}")
    # print(f"Manual test:  {'✅ PASS' if success3 else '❌ FAIL'}")
