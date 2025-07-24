<!-- # webserv

Build a HTTP server

## Concepts

1. HTTP Protocol:

    Understand HTTP/1.1 (status codes, headers, methods like GET, POST, DELETE).
    Resource: [MDN Web Docs on HTTP](https://developer.mozilla.org/en-US/docs/Web/HTTP)
    Resource: [RFC 2616 (HTTP/1.1)](https://datatracker.ietf.org/doc/html/rfc2616)

2. Networking Basics:

    Learn about sockets, non-blocking I/O, and protocols like TCP/IP.
    Resource: [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)

3. Server Configuration:

    Study how servers like NGINX use configuration files to define behavior.
    Resource: [NGINX Beginner’s Guide](https://beej.us/guide/bgnet/)

4. Poll Mechanisms:

    Learn about poll(), select(), and epoll() for handling multiple connections.

5. CGI (Common Gateway Interface):

    Understand how to execute external scripts via the server.
    Resource: [Wikipedia on CGI](https://en.wikipedia.org/wiki/Common_Gateway_Interface)

6. Error Handling and Default Pages:

    Learn to handle server errors gracefully and provide default error pages.

7. Stress Testing:

    Tools like Apache Benchmark (ab) or wrk for testing server resilience.
    Resource: [Apache Benchmark Guide](https://httpd.apache.org/docs/2.4/programs/ab.html)

## HTTP Protocol:

### What is HTTP?:

 Hypertext Transfer Protocol (HTTP) is an application-layer protocol for transmitting hypermedia documents, such as HTML. It was designed for communication between web browsers and web servers, but it can also be used for other purposes, such as machine-to-machine communication, programmatic access to APIs, and more.

HTTP follows a classical client-server model, with a client opening a connection to make a request, then waiting until it receives a response from the server. HTTP is a stateless protocol, meaning that the server does not keep any session data between two requests, although the later addition of cookies adds state to some client-server interactions.

References

### HTTP headers

Message headers are used to send metadata about a resource or a HTTP message, and to describe the behavior of the client or the server.

### HTTP request methods

Request methods indicate the purpose of the request and what is expected if the request is successful. The most common methods are ```GET``` and ```POST``` for retrieving and sending data to servers, respectively, but there are other methods which serve different purposes such as ```DELETE```.

### HTTP response status codes

Response status codes indicate the outcome of a specific HTTP request. Responses are grouped in five classes: 
- informational
- successful
- redirections
- client errors
- server errors.

## Suggested Workflow

Start Small:
    Implement a basic HTTP server that can handle simple GET requests.

Add Complexity:
    Implement POST and DELETE methods.
    Support static files and directory listings.
    Add configurations for custom error pages, port settings, and limits.

Use NGINX for Comparison:
    Set up a simple NGINX server and compare behaviors for different scenarios.

Test Extensively:
    Use curl and telnet for manual testing.
    Automate tests in Python or another scripting language.

## Tools and Resources

Books:
   "The Definitive Guide to HTTP" by David Gourley
   "Unix Network Programming" by W. Richard Stevens

Online Tutorials:
   CS50 Web Development with Python and JavaScript -->


netcat: Processing uncomplete Headersection leads to fail -> DONE
POST: wrong status code -> DONE 
POST: Bad Request: curl -v -X POST -H "Host: abc.com" localhost:8080/hello.txt "Hello world" leads to 500 
    Fixed: checkReturn() changed to allow zero size requests
POST: On any error -> delete file     -> DONE?

Long POST: yes | curl -v -X POST -H "Host: abc.com" -H "Content-Type: text/plain" --data @- http://localhost:8080/upload/hello.txt

Using @- to read from STDIN does not seem to work, closing the FD and sending data isnt working

However, this works:
yes | curl -v -X POST -H "Host: abc.com\n\nContent-Type: plain/text" --data "BODY IS HERE write something shorter or longer than body limit" http://localhost:8080/hello.txt

single POST: content was not uploaded, file was not removed on interupt
Double POST: no 407 confict response

If multipart supportet extract filename from there?

Support POST with no filename

<!-- -> add newlines -->

<!-- -> check if should be deleted -->

<!-- -> check difference between remove and unlink -> remove can delete empty respositories, unlink can not
	=> changed all to remove -->

<!-- -> remove clients before goodbye message -->

<!-- -> add "Server disconnected" instead of Client for the actual servers -->

<!-- -> checkReturn only checks for -1 because of the empty post thing, but if we are not using it for 0 as well
	should i just get rid of it and check for -1 and 0 manually since we have to check anyway?
	i just changed the checkReturn function to take the last argument as the error message for 0 if it is given, otherwise defaults to empty and returns true -->

-> [started checking] check ALL error codes

<!-- -> should stuff like "error sent" etc. keep being printed before we actually do it (in handleEpollOut)? -->

<!-- -> [switched but needs to be tested more] HTTP/1.1 will by default set the connection to "keep-alive"
	only shows the connection for close if:
	- it was requested to be closed by the Client
	- we (the server) decide to do so because we have encountered an error or smth thats not implemented
	- the response has no (valid) content length included and no chunked encoding was used -->

<!-- -> tried to upload a file (in browser) and check along with what the terminal says:
	after deleting the file it creates another request to get the same file and tells me its not found
	(deleted the default GET method from the request constructor..?) -->

<!-- -> host names and header fields must be case insensitive -->

<!-- -> A server which receives an entity-body with a transfer-coding it does
   not understand SHOULD return 501 (Unimplemented), and close the
   connection. -->

<!-- -> GET request with body-> should ignore body -->

-> If a request contains a message-body and a Content-Length is not given,
   the server SHOULD respond with 400 (bad request) if it cannot determine the length of the message

<!-- -> Messages MUST NOT include both a Content-Length header field and a
   non-identity transfer-coding. If the message does include a non-
   identity transfer-coding, the Content-Length MUST be ignored.
   When a Content-Length is given in a message where a message-body is
   allowed, its field value MUST exactly match the number of OCTETs in
   the message-body. HTTP/1.1 user agents MUST notify the user when an
   invalid length is received and detected. -->

<!-- -> Note that the absolute path cannot be empty; if none is present in the original URI,
	it MUST be given as "/" (the server root). -->

<!-- -> 1. If Request-URI is an absoluteURI, the host is part of the
     Request-URI. Any Host header field value in the request MUST be
     ignored.
   2. If the Request-URI is not an absoluteURI, and the request includes
     a Host header field, the host is determined by the Host header
     field value.
   3. If the host as determined by rule 1 or 2 is not a valid host on
     the server, the response MUST be a 400 (Bad Request) error message. -->

