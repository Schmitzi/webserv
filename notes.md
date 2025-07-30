
**FIREFOX**
**continues to accept new clients even though nothing changed, mouse just moved**
[2025-07-30 18:16:47] [7] Client accepted
[2025-07-30 18:16:47] [7] New connection from 127.0.0.1:50888
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
[2025-07-30 18:16:54] [8] Client accepted
[2025-07-30 18:16:54] [8] New connection from 127.0.0.1:50902
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
[2025-07-30 18:17:02] [9] Client accepted
[2025-07-30 18:17:02] [9] New connection from 127.0.0.1:49974

**the following browsers have problems with deleting something-> always sends another get request afterwards, which fails -> check out TODO in requestbuilder.js**

**EDGE**
[2025-07-30 19:15:30] [13] Client accepted
[2025-07-30 19:15:30] [13] New connection from 127.0.0.1:41686
[2025-07-30 19:15:30] [13] Complete request received, processing...
[2025-07-30 19:15:30] [13] Parsed Request: DELETE /upload/Hacknet.url HTTP/1.1
[2025-07-30 19:15:30] [13] Response sent (headers only)
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
[2025-07-30 19:15:30] [14] Client accepted
[2025-07-30 19:15:30] [14] New connection from 127.0.0.1:41692
[2025-07-30 19:15:30] [14] Complete request received, processing...
[2025-07-30 19:15:30] [14] Parsed Request: GET /upload/Hacknet.url HTTP/1.1
[2025-07-30 19:15:30] [14] Handling GET request for path: /upload/Hacknet.url
[2025-07-30 19:15:31] [14] Error sent: 404 Not Found
[2025-07-30 19:15:31] [14] Cleaned up and disconnected client
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
[2025-07-30 19:15:31] [14] Client accepted
[2025-07-30 19:15:31] [14] New connection from 127.0.0.1:41708
[2025-07-30 19:15:31] [14] Complete request received, processing...
[2025-07-30 19:15:31] [14] Parsed Request: GET /images/search.jpg HTTP/1.1
[2025-07-30 19:15:31] [14] Handling GET request for path: /images/search.jpg
[2025-07-30 19:15:31] [14] Sent file: local/images/search.jpg

**BRAVE**
[2025-07-30 19:17:07] [28] Client accepted
[2025-07-30 19:17:07] [28] New connection from 127.0.0.1:59380
[2025-07-30 19:17:07] [27] Complete request received, processing...
[2025-07-30 19:17:07] [27] Parsed Request: GET /js/requestbuilder.js HTTP/1.1
[2025-07-30 19:17:07] [27] Handling GET request for path: /js/requestbuilder.js
[2025-07-30 19:17:07] [27] Sent file: local/js/requestbuilder.js
[2025-07-30 19:17:25] [28] Complete request received, processing...
[2025-07-30 19:17:25] [28] Parsed Request: DELETE /upload/Hacknet.url HTTP/1.1
[2025-07-30 19:17:25] [28] Response sent (headers only)
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
[2025-07-30 19:17:26] [29] Client accepted
[2025-07-30 19:17:26] [29] New connection from 127.0.0.1:56252
[2025-07-30 19:17:26] [29] Complete request received, processing...
[2025-07-30 19:17:26] [29] Parsed Request: GET /upload/Hacknet.url HTTP/1.1
[2025-07-30 19:17:26] [29] Handling GET request for path: /upload/Hacknet.url
[2025-07-30 19:17:26] [29] Error sent: 404 Not Found
[2025-07-30 19:17:26] [29] Cleaned up and disconnected client
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
[2025-07-30 19:17:26] [29] Client accepted
[2025-07-30 19:17:26] [29] New connection from 127.0.0.1:56266
[2025-07-30 19:17:26] [29] Complete request received, processing...
[2025-07-30 19:17:26] [29] Parsed Request: GET /images/search.jpg HTTP/1.1
[2025-07-30 19:17:26] [29] Handling GET request for path: /images/search.jpg
[2025-07-30 19:17:26] [29] Sent file: local/images/search.jpg

**DUCKDUCKGO**
**duckduckgo cannot find the favicon**
[2025-07-30 19:18:38] [37] Client accepted
[2025-07-30 19:18:38] [37] New connection from 127.0.0.1:54652
[2025-07-30 19:18:38] [37] Complete request received, processing...
[2025-07-30 19:18:38] [37] Parsed Request: DELETE /upload/Hacknet.url HTTP/1.1
[2025-07-30 19:18:38] [37] Response sent (headers only)
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
[2025-07-30 19:18:38] [38] Client accepted
[2025-07-30 19:18:38] [38] New connection from 127.0.0.1:54656
[2025-07-30 19:18:39] [38] Complete request received, processing...
[2025-07-30 19:18:39] [38] Parsed Request: GET /upload/Hacknet.url HTTP/1.1
[2025-07-30 19:18:39] [38] Handling GET request for path: /upload/Hacknet.url
[2025-07-30 19:18:39] [38] Error sent: 404 Not Found
[2025-07-30 19:18:39] [38] Cleaned up and disconnected client
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
[2025-07-30 19:18:39] [38] Client accepted
[2025-07-30 19:18:39] [38] New connection from 127.0.0.1:54658
[2025-07-30 19:18:39] [38] Complete request received, processing...
[2025-07-30 19:18:39] [38] Parsed Request: GET /images/search.jpg HTTP/1.1
[2025-07-30 19:18:39] [38] Handling GET request for path: /images/search.jpg
[2025-07-30 19:18:39] [38] Sent file: local/images/search.jpg
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
[2025-07-30 19:18:41] [39] Client accepted
[2025-07-30 19:18:41] [39] New connection from 127.0.0.1:54662
[2025-07-30 19:18:41] [39] Complete request received, processing...
[2025-07-30 19:18:41] [39] Parsed Request: GET /favicon.ico HTTP/1.1
[2025-07-30 19:18:41] [39] Handling GET request for path: /favicon.ico
[2025-07-30 19:18:41] [39] Error sent: 404 Not Found
[2025-07-30 19:18:41] [39] Cleaned up and disconnected client

**FIREFOX**
[2025-07-30 19:19:39] [40] Client accepted
[2025-07-30 19:19:39] [40] New connection from 127.0.0.1:43426
[2025-07-30 19:19:39] [39] Sent file: local/css/requestbuilder.css
[2025-07-30 19:19:39] [40] Complete request received, processing...
[2025-07-30 19:19:39] [40] Parsed Request: GET /js/requestbuilder.js HTTP/1.1
[2025-07-30 19:19:39] [40] Handling GET request for path: /js/requestbuilder.js
[2025-07-30 19:19:39] [40] Sent file: local/js/requestbuilder.js
[2025-07-30 19:19:55] [40] Complete request received, processing...
[2025-07-30 19:19:55] [40] Parsed Request: DELETE /upload/Hacknet.url HTTP/1.1
[2025-07-30 19:19:55] [40] Response sent (headers only)
[2025-07-30 19:19:56] [40] Complete request received, processing...
[2025-07-30 19:19:56] [40] Parsed Request: GET /upload/Hacknet.url HTTP/1.1
[2025-07-30 19:19:56] [40] Handling GET request for path: /upload/Hacknet.url
[2025-07-30 19:19:56] [40] Error sent: 404 Not Found
[2025-07-30 19:19:56] [40] Cleaned up and disconnected client
[2025-07-30 19:19:56] [39] Complete request received, processing...
[2025-07-30 19:19:56] [39] Parsed Request: GET /images/search.jpg HTTP/1.1
[2025-07-30 19:19:56] [39] Handling GET request for path: /images/search.jpg
[2025-07-30 19:19:56] [39] Sent file: local/images/search.jpg