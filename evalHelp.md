# Fixes

## Fix for curl command
```bash
curl --resolve example.com:80:127.0.0.1 http://abc.com/   
```

### The Fix:
Need to add port to last abc.com to avoid going to REAL abc.com
```bash
curl --resolve example.com:80:127.0.0.1 http://abc.com:8080/   
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

