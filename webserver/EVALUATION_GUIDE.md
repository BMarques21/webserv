# 📋 WEBSERVER EVALUATION - FULL ASSESSMENT
## Complete Evaluation Sheet with Pass/Fail Status

---

# 🔴 CRITICAL ISSUES FOUND

## ⚠️ BLOCKING ISSUES (MUST FIX)

| Issue | Status | Impact |
|-------|--------|--------|
| CGI Not Implemented | ❌ **FAIL** | Mandatory requirement |
| Multiple Servers Not Supported | ❌ **FAIL** | Only binds to first server |
| errno check in poll() | ⚠️ **WARNING** | Line 56 - for EINTR only (acceptable) |

---

## ✅ MANDATORY PART - CODE REVIEW

### I/O Multiplexing Questions & Answers

#### 1. **What function did you use for I/O Multiplexing?**
**Answer:** `poll()`  
**Location:** [srcs/Server.cpp:53](srcs/Server.cpp#L53)

#### 2. **How does poll() work?**
**Answer:** 
- `poll()` monitors multiple file descriptors simultaneously
- Takes an array of `pollfd` structures containing file descriptors and events to monitor
- Returns when one or more file descriptors are ready for the requested I/O operation
- Can specify timeout for how long to wait
- Events monitored: POLLIN (data to read), POLLOUT (ready to write), POLLERR (error), POLLHUP (hangup)

#### 3. **Do you use only ONE poll()?**
**Answer:** ✅ YES
- Single `poll()` call in main event loop at [srcs/Server.cpp:53](srcs/Server.cpp#L53)
- No other select/poll/epoll calls in the codebase

#### 4. **How is server accept and client read/write managed?**
**Answer:**
- Server socket (`_server_fd`) is in poll array with POLLIN event
- When POLLIN on server socket → call `_acceptNewClient()`
- Client sockets added to poll array with POLLIN
- When POLLIN on client socket → call `_handleClientData()` which does `recv()`
- When data ready to send → add POLLOUT to client's events
- When POLLOUT on client socket → call `_flushClientBuffer()` which does `send()`

#### 5. **Does poll() check read AND write AT THE SAME TIME?**
**Answer:** ✅ YES
- Single poll() monitors all file descriptors
- Events can include both POLLIN and POLLOUT simultaneously
- Same loop iteration handles both read and write events
- See [srcs/Server.cpp:63-105](srcs/Server.cpp#L63-L105)

#### 6. **One read/write per client per poll()?**
**Answer:** ✅ YES  
- ONE `recv()` call per POLLIN event: [srcs/Server.cpp:194](srcs/Server.cpp#L194)
- ONE `send()` call per POLLOUT event: [srcs/Server.cpp:324](srcs/Server.cpp#L324)

---

## 🔍 CRITICAL CODE CHECKS

### ✅ recv() Error Handling - CORRECT
**Location:** [srcs/Server.cpp:194-202](srcs/Server.cpp#L194-L202)
```cpp
int bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);

if (bytes_read <= 0) {
    if (bytes_read == 0) {
        std::cout << "Client disconnected: fd=" << client_fd << std::endl;
    } else {
        std::cerr << "Error reading from client: fd=" << client_fd << std::endl;
    }
    _removeClient(client_fd);  // ✅ Client IS removed
    return;
}
```
✅ **Both 0 and -1 checked**  
✅ **Client removed on error**  
✅ **NO errno check**

### ✅ send() Error Handling - FIXED
**Location:** [srcs/Server.cpp:324-330](srcs/Server.cpp#L324-L330)
```cpp
ssize_t sent = send(client_fd, buffer.c_str(), buffer.length(), 0);
if (sent > 0) {
    buffer.erase(0, static_cast<size_t>(sent));
} else if (sent <= 0) {
    _removeClient(client_fd);  // ✅ Client IS removed
    return;
}
```
✅ **Both positive and <=0 checked**  
✅ **Client removed on error**  
✅ **NO errno check** (FIXED - previously would have caused grade 0)

### ✅ File Operations
**ifstream/ofstream used for:**
- Reading static files: [srcs/StaticFileHandler.cpp:61](srcs/StaticFileHandler.cpp#L61)
- Writing uploads: [srcs/UploadHandler.cpp:72](srcs/UploadHandler.cpp#L72)
- Reading config: [srcs/Config.cpp:263](srcs/Config.cpp#L263)

**Note:** These are disk file operations, NOT socket operations. The evaluation rule "ANY file descriptor without going through poll()" typically refers to socket file descriptors only, but clarify with evaluator if needed.

---

## 🧪 CONFIGURATION TESTS

### Test 1: Multiple Servers with Different Ports
**Config modification needed:**
```nginx
server {
    listen 8080;
    host 127.0.0.1;
    server_name localhost;
    location / {
        root www;
        index index.html;
        allowed_methods GET POST DELETE HEAD PUT;
    }
}

server {
    listen 8081;
    host 127.0.0.1;
    server_name localhost;
    location / {
        root site;
        index index.html;
        allowed_methods GET POST;
    }
}
```

**Test:**
```bash
curl http://localhost:8080/
curl http://localhost:8081/
```

### Test 2: Multiple Servers with Different Hostnames
```bash
curl --resolve example.com:8080:127.0.0.1 http://example.com:8080/
curl --resolve test.com:8080:127.0.0.1 http://test.com:8080/
```

### Test 3: Default Error Pages
**Current config:**
```nginx
error_page 404 /errors/404.html;
error_page 500 /errors/500.html;
```

**Test:**
```bash
curl -i http://localhost:8080/nonexistent  # Should return custom 404
```

### Test 4: Client Body Size Limit
**Current config:** `client_max_body_size 2147483648;` (2GB)

**Test:**
```bash
# Small body (should work)
curl -X POST -H "Content-Type: plain/text" --data "SMALL BODY" http://localhost:8080/upload

# Large body exceeding limit (should return 413)
python3 -c "print('A' * 3000000000)" | curl -X POST -H "Content-Type: plain/text" --data-binary @- http://localhost:8080/upload
```

### Test 5: Routes to Different Directories
**Current config has:**
- `/` → www
- `/upload` → www with upload_path uploads

**Test:**
```bash
curl http://localhost:8080/
curl http://localhost:8080/upload
```

### Test 6: Default File for Directory
**Current config:** `index index.html;`

**Test:**
```bash
curl http://localhost:8080/  # Should serve index.html
```

### Test 7: Allowed Methods per Route
**Current config:**
- `/` → GET, POST, DELETE, HEAD, PUT
- `/upload` → GET, POST, DELETE, HEAD, PUT
- `/cgi-bin` → GET, POST

**Test:**
```bash
# Allowed
curl -X GET http://localhost:8080/
curl -X POST http://localhost:8080/

# Test DELETE on upload (allowed)
curl -X DELETE http://localhost:8080/upload/somefile.txt

# Test PUT on cgi-bin (not allowed)
curl -X PUT http://localhost:8080/cgi-bin/test.py  # Should return 405
```

---

## 🌐 BASIC FUNCTIONALITY TESTS

### Using curl:

```bash
# GET request
curl -i http://localhost:8080/

# POST request (upload)
echo "test content" > test.txt
curl -X POST -F "file=@test.txt" http://localhost:8080/upload

# DELETE request
curl -X DELETE http://localhost:8080/upload/test.txt

# UNKNOWN method (should not crash)
curl -X PATCH http://localhost:8080/

# HEAD request
curl -I http://localhost:8080/

# Check status codes
curl -o /dev/null -w "%{http_code}\n" http://localhost:8080/  # 200
curl -o /dev/null -w "%{http_code}\n" http://localhost:8080/nonexistent  # 404
```

### Using telnet:

```bash
telnet localhost 8080
GET / HTTP/1.1
Host: localhost
Connection: close

# Press Enter twice
```

---

## 🔧 CGI TESTS

**Current Status:** CGI implementation is TODO (see [srcs/Server.cpp:428](srcs/Server.cpp#L428))

**If CGI is implemented, test:**
```bash
# GET with CGI
curl http://localhost:8080/cgi-bin/test.py

# POST with CGI
curl -X POST -d "param=value" http://localhost:8080/cgi-bin/test.py

# Test error handling
# - Script with infinite loop
# - Script with syntax error
# - Non-existent script
```

---

## 🌍 BROWSER TESTS

1. Open browser to `http://localhost:8080/`
2. Open Developer Tools (F12) → Network tab
3. Refresh page
4. Check:
   - ✅ Request headers sent
   - ✅ Response headers received
   - ✅ Status codes correct
   - ✅ Static website loads completely
   - ✅ CSS, JS, images load

5. Test scenarios:
   - Wrong URL: `http://localhost:8080/wrongpage` → Should show 404
   - Directory listing (if enabled): `http://localhost:8080/upload/`
   - Redirected URL (if configured)

---

## ⚡ PORT CONFIGURATION TESTS

### Test 1: Multiple Ports, Different Sites
**Config:**
```nginx
server {
    listen 8080;
    location / { root www; }
}
server {
    listen 8081;
    location / { root site; }
}
```

**Test in browser:**
- `http://localhost:8080/` → shows www content
- `http://localhost:8081/` → shows site content

### Test 2: Same Port Multiple Times (Should FAIL)
**Config:**
```nginx
server {
    listen 8080;
}
server {
    listen 8080;  # Duplicate!
}
```

**Expected:** Server should fail to start or give error

### Test 3: Multiple Server Instances with Common Ports
Try starting two instances of webserv with configs that have overlapping ports.
**Expected:** Second instance should fail to bind

---

## 💪 SIEGE STRESS TESTS

### Installation:
```bash
# Linux
sudo apt-get install siege

# macOS
brew install siege
```

### Tests:

#### 1. Basic Availability Test (>99.5% required)
```bash
# Create simple test page if needed
echo "<html><body>Test</body></html>" > www/test.html

# Run siege benchmark mode
siege -b -t60S http://localhost:8080/test.html

# Check output:
# Availability: should be > 99.5%
```

#### 2. Memory Leak Test
```bash
# Terminal 1: Monitor memory
watch -n 1 'ps aux | grep webserv | grep -v grep'

# Terminal 2: Run continuous siege
siege -b -c10 -t300S http://localhost:8080/

# Memory should NOT increase indefinitely
```

#### 3. No Hanging Connections
```bash
# Run extended test
siege -b -c50 -t600S http://localhost:8080/

# Check connections
netstat -an | grep :8080 | grep ESTABLISHED | wc -l
# Should not grow indefinitely

# Check for TIME_WAIT
netstat -an | grep :8080 | grep TIME_WAIT | wc -l
# Normal to have some, but shouldn't accumulate
```

#### 4. Indefinite Running
```bash
# Run siege indefinitely (Ctrl+C to stop when satisfied)
siege -b http://localhost:8080/

# Server should handle requests without restart
# No crashes, no hangs
```

---

## ✅ COMPILATION CHECK

```bash
# Clean build
make clean
make

# No errors, no warnings (with -Wall -Wextra -Werror)

# Run make again
make
# Output: "Nothing to be done for 'all'"
# ✅ No re-linking
```

---

## 🎯 BONUS (Only if mandatory is perfect)

### 1. Cookies and Sessions
- Test if server maintains session state
- Check Set-Cookie headers
- Verify session persistence across requests

### 2. Multiple CGI Systems
- Test .php scripts
- Test .py scripts
- Test .pl scripts
- Each should work with appropriate interpreter

---

## 📋 EVALUATION QUESTIONS TO PREPARE

### HTTP Server Basics:
**Q: Explain the basics of an HTTP server**  
**A:** 
- Listens on a port for incoming connections
- Accepts TCP connections from clients (browsers, curl, etc.)
- Receives HTTP requests (method, URI, headers, body)
- Parses and processes requests
- Generates appropriate HTTP responses (status code, headers, body)
- Sends responses back to clients
- Handles multiple clients simultaneously using I/O multiplexing
- Manages persistent connections and timeouts

### poll() Explanation:
**Q: How does poll() work?**  
**A:**
- System call that monitors multiple file descriptors
- Takes array of `pollfd` structures with: fd, events (POLLIN/POLLOUT), revents
- Blocks until one or more FDs are ready or timeout occurs
- Returns number of FDs ready
- Check revents to see which events occurred on each FD
- More scalable than select() (no FD_SETSIZE limit)

### Architecture:
**Q: Show code flow from poll() to read/write**  
**A:**
```
Main loop (Server::run):
  └─> poll(&_poll_fds[0], ...)  // Line 53
      └─> Loop through _poll_fds
          ├─> If POLLIN on server_fd
          │   └─> _acceptNewClient()  // Line 85
          │
          ├─> If POLLIN on client_fd
          │   └─> _handleClientData()  // Line 88
          │       └─> recv()  // Line 194
          │
          └─> If POLLOUT on client_fd
              └─> _flushClientBuffer()  // Line 101
                  └─> send()  // Line 324
```

---

## ⚠️ CRITICAL CHECKLIST (MUST PASS)

- [x] Only ONE poll() in main loop
- [x] poll() checks read AND write simultaneously
- [x] One recv/send per client per poll iteration
- [x] recv() removes client on error (0 or -1)
- [x] send() removes client on error (<=0)
- [x] NO errno checks after recv/send
- [x] Compilation without re-link
- [x] No crashes on unknown requests
- [ ] Socket operations go through poll() *(disk files are exception)*

---

## 🎓 FINAL NOTES FOR EVALUATION

1. **Start server:** `./webserv config/webserv.conf`
2. **Monitor logs:** Watch terminal output for request handling
3. **Be ready to explain:** Code flow, design decisions, poll() mechanism
4. **Have test commands ready:** curl commands, browser tests
5. **Know your status codes:** 200, 201, 204, 400, 404, 405, 413, 500, 501
6. **Memory management:** Show no leaks with valgrind if asked
7. **Configuration:** Understand every line in webserv.conf

---

## 🚀 Quick Test Commands

```bash
# Start server
./webserv config/webserv.conf

# Basic tests
curl http://localhost:8080/
curl -X POST -F "file=@test.txt" http://localhost:8080/upload
curl -X DELETE http://localhost:8080/file.txt
curl -X PATCH http://localhost:8080/  # Should not crash
curl http://localhost:8080/nonexistent  # Should return 404

# Stress test (if siege installed)
siege -b -t30S http://localhost:8080/

# Memory check
valgrind --leak-check=full ./webserv config/webserv.conf
```

---

**Good luck with your evaluation! 🎉**
