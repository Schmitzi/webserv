**CHANGES**

<!-- -> added 414 - URI Too Long
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

-> with the comparison I found out that nginx has a new line after each html response so i added that to ours -->

[nm -u ./webserv]

-> change atoi to std::...

-> can we use epoll_create1? (jakob said we should use 2, subject only specifies epoll_create)

<!-- -> if all bind() fail: dont start webserv -->

<!-- -> check error_pages in config (should be >= 400) -->

-> GET /dev/urandom returns 404, should not be 404

-> GET /for location / without GET in limit_except still returns index.html

<!-- -> combine upload_store with location name and root -->

<!-- -> combine error_pages with root -->

<!-- -> add check for index in config to only take 1 file or change it to be abale to use more than one (just dont ignore) -->

-> open fails because of 403 but sends 500 because it wasnt checked properly
	maybe because of adding to output string instead of setting once and returning (dont continue?) -> 423 tryLockFile save code and check afterwards

<!-- -> if no default_server specified take first one as default -->

-> check filesize (bytes to be received) before receiving bytes!

CGI:

-> cgi should be run in the correct direcotry (where the script is located)

-> succesful cgi request -> FEHLER -> everything shit

-> path info not working

-> maybe use cgi headers??? would be cool but not a must

-> cgi exit with not 0 should be BAD_GATEWAY and not INTERNAL_SERVER_ERROR