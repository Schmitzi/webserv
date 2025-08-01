**CHANGES**

-> added 414 - URI Too Long
-> added 408 - Request Timeout for Clients, also works if eg. nc localhost 8080 gets CTRL-C'd
-> changed '=' to '+=' for each output message
-> deleted all the printConfig functions
-> changed the statusCodes for closing the connection a little because I did some more research

in the response part:
-> each response now starts with "HTTP/1.1" and the correct statusCode + text (some were hardcoded)
-> second line is the Server + third line has the current time in nginx-format (i think?)
-> added "keep-alive" printing again, found out that nginx actually does print it

idk if you know already, but with this we can check the differences between our webserv and nginx:
diff <(curl -v http://localhost:8080/ > webservOut.txt) <(curl -v http://localhost:80/ > nginxOut.txt)

-> with the comparison I found out that nginx has a new line after each html response so i added that to ours