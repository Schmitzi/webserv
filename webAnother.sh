#!/bin/bash

HOST="localhost"
PORT="8080"
BASE_URL="http://$HOST:$PORT"
TMP_FILE="upload_test.txt"
BIG_BODY_FILE="big_body.tmp"
PASS_COUNT=0
TOTAL_COUNT=0

GREEN="\033[0;32m"
RED="\033[0;31m"
RESET="\033[0m"

function print_header() {
  echo -e "\n===> $1"
}

function run_test() {
  local description="$1"
  shift
  TOTAL_COUNT=$((TOTAL_COUNT + 1))
  print_header "$TOTAL_COUNT. $description"
  output=$(eval "$@" 2>&1)
  status=$?

  if [ $status -eq 0 ]; then
    echo -e "${GREEN}✅ Success${RESET}"
    PASS_COUNT=$((PASS_COUNT + 1))
  else
    echo -e "${RED}❌ Failed${RESET}"
    echo "$output"
  fi
}

# ========== PREP ==========
echo "Webserv Testing Started at $BASE_URL"
echo "====================================="

echo "this is a test upload" > "$TMP_FILE"
dd if=/dev/zero bs=1M count=2 of=$BIG_BODY_FILE status=none

# ========== TESTS ==========

run_test "Basic GET Request" \
  "curl -fs $BASE_URL/"

run_test "Static File Request (/index.html)" \
  "curl -fs $BASE_URL/index.html"

run_test "404 Not Found" \
  "! curl -fs $BASE_URL/nope.html"

run_test "403 Forbidden (no permission dir)" \
  "! curl -fs $BASE_URL/forbidden_dir/"

run_test "Autoindex Directory" \
  "curl -fs $BASE_URL/listing/ | grep -q '<title>Index of'"

run_test "POST Form Data" \
  "curl -fs -X POST -d 'foo=bar' $BASE_URL/form-handler"

run_test "File Upload (text/plain)" \
  "curl -fs -X POST --data-binary @$TMP_FILE $BASE_URL/upload"

run_test "Multipart Form Upload" \
  "curl -fs -F 'file=@$TMP_FILE' $BASE_URL/upload"

run_test "DELETE Uploaded File" \
  "curl -fs -X DELETE $BASE_URL/upload/$TMP_FILE"

run_test "HTTP Redirect (301/302)" \
  "curl -fs -L $BASE_URL/redirectme"

run_test "Virtual Host" \
  "curl -fs -H 'Host: myvirtualhost' $BASE_URL/"

run_test "CGI Execution (GET)" \
  "curl -fs $BASE_URL/cgi-bin/hello.py?param=value"

run_test "CGI Execution (POST)" \
  "curl -fs -X POST -d 'input=abc' $BASE_URL/cgi-bin/hello.py"

run_test "Chunked Transfer Encoding" \
  "curl -fs -H 'Transfer-Encoding: chunked' --data-binary @$TMP_FILE $BASE_URL/chunked"

run_test "Method Not Allowed (PUT)" \
  "! curl -fs -X PUT $BASE_URL/"

run_test "Keep-Alive Header (Connection reuse)" \
  "curl -fs --header 'Connection: keep-alive' $BASE_URL/"

# run_test "Large Request Body (2MB)" \
#   "! curl -fs -X POST --data-binary @$BIG_BODY_FILE $BASE_URL/upload"

# run_test "Invalid HTTP Method (via netcat workaround)" \
#   "! printf 'FOO / HTTP/1.1\r\nHost: localhost\r\n\r\n' | nc $HOST $PORT"

# run_test "Unsupported HTTP Version" \
#   "! printf 'GET / HTTP/0.9\r\nHost: localhost\r\n\r\n' | nc $HOST $PORT"

# run_test "Missing Host Header (HTTP/1.1)" \
#   "! printf 'GET / HTTP/1.1\r\n\r\n' | nc $HOST $PORT"

# run_test "Multiple Concurrent GET Requests (x10)" \
#   "seq 1 10 | xargs -n1 -P10 curl -fs $BASE_URL/ > /dev/null"

# ========== CLEANUP ==========
rm -f "$TMP_FILE" "$BIG_BODY_FILE"

# ========== RESULTS ==========
echo -e "\n====================================="
echo -e "✅ ${GREEN}$PASS_COUNT${RESET} / $TOTAL_COUNT tests passed"
echo "====================================="
