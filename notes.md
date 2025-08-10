# Notes

## Points from Eval

<!-- - Add Dependencies -->
<!-- - Get method in the request IS case sensitive. -->
<!-- - Check "absolute form" in netcat -->
<!-- - Is checkRaw function correct? -->
<!-- - Slashes need to be encoded -->
<!-- - Remove blank Content-Type from DELETE request (NOCONT?) -->
- POST localhost:8080 gives 409 error -> maybe this is correct?
<!-- - Excess found in netcat (possible requires fcntl for true blocking) -->
<!-- - PHP CGI results in 502 Bad Gateway -> add fullpath to scriptname (getcwd() + abspath) -->
- Timeout CGI broken (_timeout was not initialized)
- Chunked should use the first line as the Content-Length

- Double check all changes were merged correctly
<!-- - Block symlinks (O_NOFOLLOW) -> /dev/urandom as conf results in SegFault -->
