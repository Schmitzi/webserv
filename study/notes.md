# webserv

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
   CS50 Web Development with Python and JavaScript
