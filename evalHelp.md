# Fixes

## Fix for curl command
```bash
curl --resolve abc.com:80:127.0.0.1 http://abc.com/   
```

### The Fix:
Need to add port to last abc.com to avoid going to REAL abc.com
```bash
curl --resolve abc.com:80:127.0.0.1 http://abc.com:8080/   
```

## POST with curl
```bash
curl -X POST -H "Content-Type: plain/text" --data "BODY IS HERE write something shorter or longer than body limit"
```

### The Fix:
Specify the location of upload
```bash
curl -X POST -H "Content-Type: plain/text" --data "BODY IS HERE write something shorter or longer than body limit" http://localhost:8080/upload/test.txt
```

## Siege

Here is a command to test the servers resilience and responsivity
```bash
siege --delay=0.5 --file=tests/urls.txt --internet --verbose --reps=200 --concurrent=15 --no-parser
```

## Curl Help

With curl we can specify how we want to format our requests.

### Methods

By default, curl will use GET, for example

```bash
curl localhost:8080  # GET / HTTP/1.1
```

To use a different method, we use the <b>-X</b> flag

```bash
curl -X POST localhost:8080/test.txt    # POST test.txt HTTP/1.1
```

### Version

You will have also noticed that the version <b>HTTP/1.1</b> was also added automatically. To change this we simply add <b>--httpX.X</b>, where X is the the version we want instead.

```bash
curl --http1.0 localhost:8080   # GET / HTTP/1.0
```

### Host Header

In curl, if no host header/server name is given, then curl will use the current host

```bash
[fortytwo@nixos:~/Documents/webserv]$ curl localhost:8080 -v    # -v added to show result verbosely
* Host localhost:8080 was resolved.
* IPv6: ::1
* IPv4: 127.0.0.1:8080
* Connected to localhost (127.0.0.1) port 8080
> GET / HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/8.14.1
> Accept: */*
> 
* Request completely sent off
< HTTP/1.1 200 OK
< Content-Type: text/html
< Content-Length: 2228
< Server: WebServ/1.0
< Access-Control-Allow-Origin: *
< 
```

### Uploading and Downloading
To upload something with curl you can use a variation of this script

```bash
curl -d @test.txt localhost:8080/test.txt
```

Downloading is much simpler. however depending on the configuration, the file may need to be in specific directory to be accessed
```bash
curl localhost:8080/upload/test.txt   # GET /upload/test.txt HTTP/1.1
```

To  save the file, add the -O flag. This will save to current directory
```bash
curl -O localhost:8080/test.txt
```

You can also change the name
```bash
curl -o something.txt  localhost:8080/test.txt
```

# Here are the "points to check"

## Configuration

In the configuration file, check whether you can do the following and test the result:

- Search for the HTTP response status codes list on the internet. During this evaluation, if any status codes is wrong, don't give any related points.
- Setup multiple servers with different ports.
- Setup multiple servers with different hostnames (use something like: `curl --resolve example.com:80:127.0.0.1 http://example.com/`).
- Setup default error page (try to change the error 404).
- Limit the client body (use: `curl -X POST -H "Content-Type: plain/text" --data "BODY IS HERE write something shorter or longer than body limit"`).
- Setup routes in a server to different directories.
- Setup a default file to search for if you ask for a directory.
- Setup a list of methods accepted for a certain route (e.g., try to delete something with and without permission).

## Basic checks

Using telnet, curl, prepared files, demonstrate that the following features work properly:

- GET, POST and DELETE requests should work.
- UNKNOWN requests should not result in a crash.
- For every test you should receive the appropriate status code.
- Upload some file to the server and get it back.


## Check CGI

Pay attention to the following:

- The server is working fine using a CGI.
- The CGI should be run in the correct directory for relative path file access.
- With the help of the students you should check that everything is working properly. You have 
    to test the CGI with the "GET" and "POST" methods.
- You need to test with files containing errors to see if the error handling works properly.
    You can use a script containing an infinite loop or an error; you are free to do whatever 
    tests you want within the limits of acceptability that remain at your discretion. The group 
    being evaluated should help you with this.

The server should never crash and an error should be visible in case of a problem.

## Check with a browser

- Use the reference browser of the team. Open the network part of it, and try to connect to the 
    server using it.
- Look at the request header and response header.
- It should be compatible to serve a fully static website.
- Try a wrong URL on the server.
- Try to list a directory.
- Try a redirected URL.
- Try anything you want to.

## Port issues

- In the configuration file setup multiple ports and use different websites.
    Use the browser to ensure that the configuration works as expected and shows the right website.
- In the configuration, try to setup the same port multiple times. It should not work.
- Launch multiple servers at the same time with different configurations but with common ports.
    Does it work? If it does, ask why the server should work if one of the configurations isn't 
    functional. Keep going.

## Siege & stress test

- Use Siege to run some stress tests.
- Availability should be above 99.5% for a simple GET on an empty page with a siege -b on that page.
- Verify there is no memory leak (Monitor the process memory usage. It should not go up indefinitely).
- Check if there is no hanging connection.
- You should be able to use siege indefinitely without having to restart the server (take a look at siege -b).