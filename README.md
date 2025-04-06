# webserv
The purpose of this project is to build a webserver in C++98 that handles request and parses configurations files

Team : [Michael](https://github.com/Schmitzi)

## Index

- What is CGI
- Useful functions
  - What is socket()?
  - What is bind()?
  - What is listen()?
  - What is accept()?

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