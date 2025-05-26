import http.client
import os

def run_cgi_with_file_arg_test(host="localhost", port=8080):
    # Expected data
    expected_file_path = "local/cgi-bin/data.txt"
    expected_content = "Hello from the test file!\n"

    # Create the test file (the one to be passed as argv[1])
    os.makedirs("local/cgi-bin", exist_ok=True)
    with open(expected_file_path, "w") as f:
        f.write(expected_content)

    # Create the CGI script
    cgi_script_path = "local/cgi-bin/echo_file.py"
    with open(cgi_script_path, "w") as f:
        f.write("""#!/usr/bin/env python3
import sys
print("Content-Type: text/plain\\n")
try:
    with open(sys.argv[2], 'r') as file:
        print(file.read())
except Exception as e:
    print("Error:", e)
""")

    os.chmod(cgi_script_path, 0o755)  # Make it executable

    # Send the request
    conn = http.client.HTTPConnection(host, port)
    conn.request("GET", "/cgi-bin/echo_file.py")
    response = conn.getresponse()
    body = response.read().decode()
    conn.close()

    print("Status:", response.status)
    print("Response Body:\n", body)

    assert response.status == 200, f"Expected HTTP 200 OK, got {response.status}"
    assert expected_content.strip() in body, "Expected file content not found in response."

    print("✅ CGI with file-as-arg test passed!")


if __name__ == "__main__":
    run_cgi_with_file_arg_test()

#check if it actually gets the file as an argument
# import http.client
# import os

# def run_cgi_with_file_arg_test(host="localhost", port=8080):
#     # Expected data
#     expected_file_path = "local/cgi-bin/data.txt"
#     expected_content = "Hello from the test file!\n"

#     # Create the test file (the one to be passed as argv[1])
#     os.makedirs("local/cgi-bin", exist_ok=True)
#     with open(expected_file_path, "w") as f:
#         f.write(expected_content)

#     # Create the CGI script
#     cgi_script_path = "local/cgi-bin/echo_file.py"
#     with open(cgi_script_path, "w") as f:
#         f.write("""#!/usr/bin/env python3
# import sys
# print("Content-Type: text/plain\\n")
# print("ARGV:", sys.argv)
# """)

#     os.chmod(cgi_script_path, 0o755)  # Make it executable

#     # Send the request
#     conn = http.client.HTTPConnection(host, port)
#     conn.request("GET", "/cgi-bin/echo_file.py")
#     response = conn.getresponse()
#     body = response.read().decode(errors='ignore')
#     conn.close()

#     print("Status:", response.status)
#     print("Response Body:\n", body)

#     assert response.status == 200, f"Expected HTTP 200 OK, got {response.status}"
#     assert "data.txt" in body, "Expected file argument not found in CGI script output."

#     print("✅ CGI with file-as-arg test passed!")


# if __name__ == "__main__":
#     run_cgi_with_file_arg_test()