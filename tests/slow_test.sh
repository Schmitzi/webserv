#!/bin/bash

(
  for i in {1..1000}; do
    printf 'A%.0s' {1..1000}  # 1000 A's per line = 1KB
    usleep 10000              # sleep 100ms between chunks
  done
) | curl -v -X POST \
     -H "Host: abc.com" \
     -H "Content-Type: text/plain" \
     --data-binary @- \
     http://localhost:8080/upload/hello.txt


{
  printf 'POST /upload/hello.txt HTTP/1.1\r\n'
  printf 'Host: abc.com\r\n'
  printf 'Content-Length: 1000000\r\n'
  printf 'Content-Type: Multipart/form-data; boundary="xyz"\r\n'
  printf '\r\n'

  i=0
  while [ $i -lt 10 ]; do
    printf '--xyz\r\nA%.0s' $(seq 1 1000)
    sleep 1
    i=$((i + 1))
  done
  printf '--xyz--\r\n'
} | nc localhost 8080