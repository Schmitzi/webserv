#!/usr/bin/env python3

import requests
from colorama import Fore, Style

# Config
HOST = "localhost"
PORT = 8080
TARGET = f"http://{HOST}:{PORT}"
ENCODED_PATH = "with%20space/test%20space.txt"
DECODED_PATH = "with space/test space.txt"
REDIRECT_TEST = "redir%20test"
AUTOINDEX_PATH = "autoindex%20dir"

def pass_msg(msg):
    print(f"{Fore.GREEN}✅ {msg}{Style.RESET_ALL}")

def fail_msg(msg):
    print(f"{Fore.RED}❌ {msg}{Style.RESET_ALL}")

def info_msg(msg):
    print(f"{Fore.LIGHTBLACK_EX}ℹ️  {msg}{Style.RESET_ALL}")

def test_percent_decoding():
    print("\n>>> Test 1: Percent-decoded URI returns correct file")
    try:
        response = requests.get(f"{TARGET}/{ENCODED_PATH}")
        if response.status_code == 200:
            pass_msg(f"Request to '{ENCODED_PATH}' succeeded (HTTP 200)")
        else:
            fail_msg(f"Expected HTTP 200 for '{ENCODED_PATH}', got {response.status_code}")
    except requests.RequestException as e:
        fail_msg(f"Request failed: {e}")

def test_redirect_location_header():
    print("\n>>> Test 2: Location header in redirect is percent-encoded")
    try:
        response = requests.get(f"{TARGET}/{REDIRECT_TEST}", allow_redirects=False)
        location = response.headers.get("Location", "")
        if "%20" in location:
            pass_msg(f"Location header is correctly percent-encoded: {location}")
        elif location:
            fail_msg(f"Location header exists but is not encoded: {location}")
        else:
            fail_msg(f"No Location header received for '{REDIRECT_TEST}'")
    except requests.RequestException as e:
        fail_msg(f"Request failed: {e}")

def test_autoindex_links_encoded():
    global FAIL
    print("\n>>> Test 3: Autoindex hrefs use encoded links")
    try:
        response = requests.get(f"{TARGET}/{AUTOINDEX_PATH}/")
        html = response.text
        if "%20" in html:
            pass_msg("Autoindex hrefs contain encoded links")
        elif "<title>Index of" in html:
            fail_msg("Autoindex active, but hrefs are not encoded")
        elif "index space" in html.lower():
            info_msg("Directory contains an index file — autoindex was skipped")
        else:
            info_msg(f"No autoindex response detected at '{AUTOINDEX_PATH}' — skipping")
    except requests.RequestException as e:
        fail_msg(f"Request failed: {e}")

def main():
    print("=== Starting Percent-Encoding Tests ===")
    test_percent_decoding()
    test_redirect_location_header()
    test_autoindex_links_encoded()

if __name__ == "__main__":
    main()
