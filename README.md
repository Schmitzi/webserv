# Webserv
The purpose of this project is to build a webserver in C++98 that handles request and parses configurations files

Team : [Michael](https://github.com/Schmitzi) | [Lilly](https://github.com/waterlilly)

## Index
- How do I use Webserv?
- What is CGI?
- Useful functions
  - What is socket()?
  - What is bind()?
  - What is listen()?
  - What is accept()?

# How do I use Webserv?

To use webserv, you first need to compile it

```bash
make start
```

This will clear the objects, recompile the binary and start the server with the default configuration.

Then open a second terminal and enter:
```bash
nc localhost 8080
```

This will start the connection to the server.

After this, you have 3 options to test the server. The vaild requests are GET, POST and DELETE.

## GET

GET returns the contents of a file. For example:

```bash
GET / HTTP/1.1
```

This will return the index.html stored at /local/ aswell as a header containing some information about the file.

You can also specify a different file such as:

```bash
GET css/styles.css HTTP/1.1
```

This will display the contents of the given CSS file

## POST

POST allows us to create a file on the server. We can use it in the following ways:

```bash
POST test.txt HTTP/1.1
```

This will create a file under /local/upload/ called text.txt

If we want to fill this with text, we need to use a different format.

```bash
POST test.txt?value=test HTTP/1.1
```

With this format, we can specify the body and write it into the file we created.

## DELETE

The final request is pretty self explanitory. 

```bash
DELETE test.txt HTTP/1.1
```

This function will delete the specified file. At the moment, this is limited to only being able to delete files located in /local/upload because I have already borked some files and deleting files this way means Ctrl+Z is not possible.....

# What is CGI (Common Gateway Interface)?

The Common Gateway Interface (CGI) is a standard protocol that defines how web servers communicate with external programs to dynamically generate content for web users.

## How CGI Works

A client sends an HTTP request to the web server for a CGI resource
The web server identifies the request as a CGI request (typically by the file's location or extension)
Instead of sending the file directly to the client, the server executes the CGI program
The CGI program processes the request, possibly accessing databases or performing other operations
The CGI program generates output (typically HTML) which is sent back to the web server
The web server relays this output to the client as the HTTP response

## Key Features of CGI

- <b>Environment Variables</b>: The web server passes information to CGI scripts through environment variables like:
  - REQUEST_METHOD
  - QUERY_STRING
  - CONTENT_LENGTH

- <b>Standard Input/Output</b>: Request data can be read from standard input (stdin), and responses are written to standard output (stdout)
- <b>Headers</b>: CGI programs must output proper HTTP headers before the content, with a blank line separating headers from the body
- <b>Content Types</b>: The "Content-type" header tells browsers how to interpret the data (e.g., "text/html", "image/jpeg")

## CGI in Modern Web Development
While CGI has been largely replaced by more efficient technologies in high-traffic environments (like FastCGI, WSGI, application servers), it's still valuable to understand as it forms the foundation of web application development concepts. Many modern frameworks still use CGI principles even if they don't use the exact implementation.

## Example program
```python
#!/usr/bin/env python3
import os
import sys

# Print headers (optional)
print("Content-Type: text/html")
print()  # Extra newline to separate headers from body

# Access environment variables
print("<html><body>")
print("<h1>Hello from CGI!</h1>")
print(f"<p>Method: {os.environ.get('REQUEST_METHOD')}</p>")
print(f"<p>Query String: {os.environ.get('QUERY_STRING')}</p>")

# Read input if any
input_data = sys.stdin.read()
if input_data:
    print(f"<p>Input Data: {input_data}</p>")

print("</body></html>")
```

This can then be called with:
```bash
curl -i "http://localhost:8080/scripts/hello.py?name=World"
```

And our output will look something like this:
```bash
HTTP/1.1 200 OK
Content-Type: text/plain
Content-Length: 104
Server: WebServ/1.0
Connection: keep-alive

<html><body>
<h1>Hello from CGI!</h1>
<p>Method: GET</p>
<p>Query String: name=World</p>
</body></html>
```
<br>

## Testing CGI in the Terminal

While using CGI in the browser will render a beautiful clean image, we can also test it in the terminal using ```nc```
```bash
GET /cgi-bin/hello.py HTTP/1.1\r\nHost: localhost\r\n\r\n | nc localhost 8080
```
<br>


# Useful functions

## What is socket()?
socket() is a syscall found at ```<sys/socket.h>``` and is defined as:
```cpp
int socket(int domain, int type, int protocol);
```
The function returns -1 in case of an error, or it returns a file descriptor assigned to a socket in case of success.

The first parameter, ```domain``` refers to the protocol the socket will use for communication. It may be one of the following.
 - AF_UNIX, AF_LOCAL - Local communication
 - AF_INET - IPv4 Internet Protocols
 - AF_INET6 - IPv6 Internet Protocols
 - AF_IPX - IPX Novell protocols
  
The second parameter, ```type```, specifies if the communication will be connectionless or persistent. Not all ```types``` are compatible with all ```domains```

Some examples of ```types``` are:

- SOCK_STREAM - Two-way reliable communication (TCP)
- SOCK_DGRAM - Connectionless, unreliable (UDP)

There is usually only one ```protocol``` for each ```type```, so the value 0 can be used.


## What is bind()?

Once we have a socket, we need to use ```bind``` to assign an IP address and port to the socket.

The signature of the ```bind``` function is:

```cpp
int bind(int sockfd, const sockaddr *addr, socklen_t addrlen);
```

```bind()``` is similar to ```socket()``` in that it returns -1 in case of an error. In case of success it returns 0.

```sockfd``` refers to the file descriptor we want to assign an address to. For us, it will be the file descriptor returned by ```socket```.

```addr``` is a struct used to specify the address we want to assign to the socket. The exact struct that needs to be used to define the address, varies by protocol.

Since we are going to use to use IP for this server, we will use ```sockaddr_in```:
```cpp
struct sockaddr_in {
	sa_family_t		sin_family;	// address family: AF_INET
	in_port_t		sin_port;	// port in network byte order
	struct in_addr	sin_addr;	// internet address
}
```

```addrlen``` is just the ```size()``` of ```addr```

## What is listen()?

```listen()``` marks a socket as passive, meaning the socket will be used to accept connections. The signature is:

```cpp
int listen(int sockfd, int backlog);
```

Again, returns -1 in case of error and 0 in case of success.

```sockfd```is the file descriptor of the socket and ``backlog``` is the maximum number of connections that will be queued befor connections start being refused.


## What is accept()?

```accept()``` extracts an element from a queue of connections (The queue created by ```listen()```) for a socket. The signature is:

```cpp
int accept(int sockfd, sockaddr *addr, socklen_t *addrlen);
```

The function will return -1 if there is an error. On success, it will return a file descriptor for the connection.

The argument list is similar to the one for ```bind()```, with one difference. ```addrlen``` is now a value result argument.
It expects a pointer to an int that will be the size of ```addr```. After the function is executed, the int refered by ```addrlen``` will be set to the size of the peer address.


## Sources
[Building a simple server with C++ - adrian.ancona](https://ncona.com/2019/04/building-a-simple-server-with-cpp/)