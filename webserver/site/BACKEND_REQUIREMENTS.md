# Webserv Backend Requirements & Implementation Guide

## Project Overview

**Webserv** is an HTTP/1.1 server implementation in C++ 98 for the 42 School project. This document contains all mandatory and bonus requirements along with implementation guidance.

---

## Table of Contents
1. [Mandatory Requirements](#mandatory-requirements)
2. [Configuration File Guide](#configuration-file-guide)
3. [HTTP Methods](#http-methods)
4. [CGI Implementation](#cgi-implementation)
5. [Error Handling](#error-handling)
6. [Testing & Stress Testing](#testing--stress-testing)
7. [Bonus Features](#bonus-features)
8. [Frontend Integration Points](#frontend-integration-points)

---

## Mandatory Requirements

### Core Project Structure
- **Language:** C++ 98 (no C++11 or later features)
- **Executable:** `webserv` (created by Makefile)
- **Launch Command:** `./webserv [configuration_file]`
- **Default Config:** If no config provided, use default path (usually `./default.conf`)

### Makefile Requirements
Your Makefile must include these targets:
- **`make` or `make all`** - Compile the project
- **`make clean`** - Remove object files
- **`make fclean`** - Remove all compiled files
- **`make re`** - Clean and recompile

### Allowed External Functions
You MUST use ONLY these functions (no others):
```
execve, pipe, strerror, gai_strerror, errno, dup, dup2, fork, 
socketpair, htons, htonl, ntohs, ntohl, select, poll, epoll 
(epoll_create, epoll_ctl, epoll_wait), kqueue (kqueue, kevent), 
socket, accept, listen, send, recv, chdir, bind, connect, 
getaddrinfo, freeaddrinfo, setsockopt, getsockname, getprotobyname, 
fcntl, close, read, write, waitpid, kill, signal, access, stat, 
open, opendir, readdir, closedir
```

### File Structure
Required files to turn in:
- `Makefile` - Build configuration
- `*.h`, `*.hpp` - Header files
- `*.cpp` - Implementation files
- `*.tpp`, `*.ipp` - Template implementations (if needed)
- `[config files]` - Configuration files (*.conf or similar)
- `README` or documentation

---

## Configuration File Guide

### File Format
Use NGINX-style configuration syntax (inspired by NGINX, not exact copy).

### Example Configuration File

```nginx
# Main server block
server {
    # Listen on interface and port
    listen 127.0.0.1:8080;
    listen 127.0.0.1:8081;
    
    # Server name (for virtual hosts)
    server_name localhost;
    
    # Document root directory
    root /var/www/html;
    
    # Index file (served when requesting directory)
    index index.html index.htm;
    
    # Maximum client body size (for uploads)
    client_max_body_size 10M;
    
    # Error pages
    error_page 404 /404.html;
    error_page 500 /500.html;
    error_page 502 /502.html;
    error_page 503 /503.html;
    
    # Location block - root path
    location / {
        # Allowed HTTP methods
        allow_methods GET POST DELETE;
        
        # Enable directory listing
        autoindex on;
    }
    
    # Location block - uploads
    location /upload {
        allow_methods GET POST;
        
        # Directory to store uploads
        upload_dir /var/www/uploads;
    }
    
    # Location block - CGI
    location /cgi-bin {
        allow_methods GET POST;
        
        # CGI extension and interpreter
        cgi_pass .php /usr/bin/php-cgi;
        cgi_pass .py /usr/bin/python3;
    }
    
    # Location block - redirection
    location /old-page {
        return 301 /new-page;
    }
}

# Second server on different port with different content
server {
    listen 127.0.0.1:9090;
    server_name alternate;
    root /var/www/alternate;
    index index.html;
    
    location / {
        allow_methods GET;
        autoindex off;
    }
}
```

### Configuration Directives

#### Global/Server Level
| Directive | Description | Example |
|-----------|-------------|---------|
| `listen` | IP:port to listen on (can have multiple) | `listen 127.0.0.1:8080;` |
| `server_name` | Server identifier (for virtual hosts) | `server_name localhost;` |
| `root` | Document root directory | `root /var/www/html;` |
| `index` | Default file for directories | `index index.html;` |
| `client_max_body_size` | Max upload size (default 1M) | `client_max_body_size 100M;` |
| `error_page` | Custom error page | `error_page 404 /404.html;` |

#### Location Block Level
| Directive | Description | Example |
|-----------|-------------|---------|
| `allow_methods` | Allowed HTTP methods | `allow_methods GET POST DELETE;` |
| `autoindex` | Enable/disable directory listing | `autoindex on;` or `off;` |
| `return` | HTTP redirection | `return 301 /new-path;` |
| `upload_dir` | Where to store uploads | `upload_dir /var/www/uploads;` |
| `cgi_pass` | CGI extension to interpreter | `cgi_pass .php /usr/bin/php-cgi;` |

---

## HTTP Methods

### GET
- **Purpose:** Retrieve resource
- **Implementation:**
  1. Parse URL path
  2. Apply location rules (root, redirects, etc.)
  3. If file exists: send file content with proper headers
  4. If directory: serve index file or directory listing
  5. If not found: return 404

### POST
- **Purpose:** Submit data to server
- **Implementation:**
  1. Parse request headers (Content-Type, Content-Length)
  2. Read request body (respecting `client_max_body_size`)
  3. If location allows POST:
     - If `upload_dir`: save file
     - If `cgi_pass`: pass to CGI interpreter
     - Otherwise: reject with 405 Method Not Allowed
  4. Return appropriate status code

### DELETE
- **Purpose:** Remove resource
- **Implementation:**
  1. Check if location allows DELETE
  2. Parse URL path
  3. Check if file exists
  4. Delete file
  5. Return 204 No Content or 200 OK

### Other Methods
- **HEAD** - Like GET but without response body
- **PUT** - Not required but can implement (similar to POST)
- **PATCH** - Not required

---

## CGI Implementation

### What is CGI?
CGI (Common Gateway Interface) allows the server to execute external programs (PHP, Python, Perl, etc.) and return their output as HTTP responses.

### How It Works

```
HTTP Request (e.g., GET /cgi-bin/script.php?name=John)
        ↓
Server receives request
        ↓
Recognizes .php extension → matched to CGI rule
        ↓
Forks a child process (fork())
        ↓
Child process sets up environment variables
        ↓
Child executes CGI program (execve(/usr/bin/php-cgi, args, env))
        ↓
CGI program reads stdin and environment variables
        ↓
CGI program writes output to stdout
        ↓
Parent process reads output via pipe
        ↓
Server sends output as HTTP response to client
        ↓
Parent process waits for child (waitpid())
```

### Environment Variables

The CGI program expects these environment variables (must be set before execve):

```
REQUEST_METHOD      GET, POST, DELETE, etc.
SCRIPT_FILENAME     Full path to CGI script
QUERY_STRING        Everything after ? in URL
CONTENT_TYPE        From request header (for POST)
CONTENT_LENGTH      Size of request body (for POST)
HTTP_HOST           Request Host header
HTTP_USER_AGENT     Request User-Agent header
REMOTE_ADDR         Client IP address
SERVER_NAME         Your server name
SERVER_PORT         Your server port
SERVER_PROTOCOL     HTTP/1.1 or HTTP/1.0
PATH_INFO           Extra path info after script name
PATH_TRANSLATED     Translated PATH_INFO
```

### Implementation Steps

1. **Recognize CGI Request**
   ```cpp
   // In location block, check if extension matches cgi_pass rule
   if (request.path.ends_with(".php")) {
       // This is a CGI request
   }
   ```

2. **Set Up Pipes** (to communicate with CGI process)
   ```cpp
   int pipe_in[2];   // Parent writes, CGI reads from stdin
   int pipe_out[2];  // CGI writes, parent reads from stdout
   pipe(pipe_in);
   pipe(pipe_out);
   ```

3. **Fork Process**
   ```cpp
   pid_t pid = fork();
   if (pid == 0) {
       // Child process - execute CGI
   } else {
       // Parent process - wait and read output
   }
   ```

4. **Child Process - Set Up and Execute**
   ```cpp
   // In child process (pid == 0):
   close(pipe_out[0]);  // Close read end, keep write end
   close(pipe_in[1]);   // Close write end, keep read end
   
   dup2(pipe_in[0], STDIN_FILENO);    // Redirect stdin
   dup2(pipe_out[1], STDOUT_FILENO);  // Redirect stdout
   
   // Set environment variables (environ)
   char *env[] = {
       "REQUEST_METHOD=GET",
       "SCRIPT_FILENAME=/var/www/script.php",
       "QUERY_STRING=name=value",
       ...
       NULL
   };
   
   // Execute CGI interpreter
   execve("/usr/bin/php-cgi", args, env);
   ```

5. **Parent Process - Wait and Read**
   ```cpp
   // In parent process (pid > 0):
   close(pipe_in[0]);   // Close read end
   close(pipe_out[1]);  // Close write end
   
   // Write request body to CGI stdin if POST
   write(pipe_in[1], request_body, size);
   close(pipe_in[1]);   // Signal EOF
   
   // Read CGI output
   char buffer[4096];
   int bytes = read(pipe_out[0], buffer, sizeof(buffer));
   
   // Wait for child to finish
   int status;
   waitpid(pid, &status, 0);
   ```

6. **Handle CGI Output**
   - CGI writes HTTP headers + body
   - Parse headers from CGI output
   - If no Content-Length header, read until EOF (important!)
   - For chunked transfer, un-chunk it before sending to client

### Supported CGI Languages

- **PHP:** `/usr/bin/php-cgi`
- **Python:** `/usr/bin/python3`
- **Perl:** `/usr/bin/perl`
- **Shell Script:** `/bin/sh` or `/bin/bash`

### Example CGI Configuration

```nginx
location /cgi-bin {
    allow_methods GET POST;
    cgi_pass .php /usr/bin/php-cgi;
    cgi_pass .py /usr/bin/python3;
    cgi_pass .pl /usr/bin/perl;
}
```

---

## Non-Blocking Architecture

### Critical Requirement
Your server MUST:
- Use **only 1 poll() (or select/epoll/kqueue)** for ALL I/O
- **Never block** on read/write operations
- Always use poll BEFORE reading/writing
- Handle multiple clients simultaneously
- Support at least 10,000+ concurrent connections

### Architecture Pattern

```cpp
// Main event loop
while (server_running) {
    // Set up file descriptors to monitor
    for (int fd : all_file_descriptors) {
        FD_SET(fd, &read_set);
        FD_SET(fd, &write_set);
    }
    
    // Wait for events (non-blocking)
    int ret = poll(fds, nfds, timeout);
    
    if (ret > 0) {
        // Check which fds are ready
        for (int i = 0; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                // Data available to read
                read_data(fds[i].fd);
            }
            if (fds[i].revents & POLLOUT) {
                // Ready to write
                write_data(fds[i].fd);
            }
        }
    }
}
```

### I/O Multiplexing Options

You can use ANY of these (poll() is mentioned but others are equivalent):
- **poll()** - Standard, portable
- **select()** - Older but works
- **epoll()** - Linux optimized
- **kqueue()** - BSD/macOS optimized

---

## Error Handling

### HTTP Status Codes Required

| Code | Meaning | When to Return |
|------|---------|-----------------|
| 200 | OK | Successful GET/POST/DELETE |
| 201 | Created | File uploaded successfully |
| 204 | No Content | Successful DELETE, no body |
| 301 | Moved Permanently | Permanent redirect |
| 302 | Found | Temporary redirect |
| 304 | Not Modified | If-Modified-Since matched |
| 400 | Bad Request | Invalid HTTP request |
| 401 | Unauthorized | Authentication needed |
| 403 | Forbidden | Access denied |
| 404 | Not Found | File doesn't exist |
| 405 | Method Not Allowed | POST on GET-only path |
| 408 | Request Timeout | Client took too long |
| 413 | Payload Too Large | Exceeds client_max_body_size |
| 414 | URI Too Long | URL too long |
| 500 | Internal Server Error | Server error |
| 502 | Bad Gateway | CGI error |
| 503 | Service Unavailable | Server overloaded |

### Default Error Pages

Create files in your root directory:
```
/var/www/html/400.html
/var/www/html/401.html
/var/www/html/403.html
/var/www/html/404.html
/var/www/html/405.html
/var/www/html/408.html
/var/www/html/413.html
/var/www/html/500.html
/var/www/html/502.html
/var/www/html/503.html
```

Each should contain proper HTML with error message.

### Timeout Handling

- Set a reasonable timeout (e.g., 30 seconds)
- If client doesn't send complete request in time: return 408
- If CGI takes too long: kill it and return 502
- Clean up resources properly

---

## Testing & Stress Testing

### Manual Testing with curl

```bash
# Test GET
curl http://localhost:8080/

# Test POST with JSON
curl -X POST -H "Content-Type: application/json" \
     -d '{"key":"value"}' http://localhost:8080/api/test

# Test POST with file upload
curl -F "file=@image.jpg" http://localhost:8080/upload

# Test DELETE
curl -X DELETE http://localhost:8080/resource/1

# Test with custom headers
curl -H "X-Custom: value" http://localhost:8080/

# Test redirection
curl -L http://localhost:8080/old-page

# Test with telnet (raw HTTP)
telnet localhost 8080
GET / HTTP/1.1
Host: localhost

```

### Stress Testing

Test with multiple concurrent clients:

```bash
# Using Apache Bench
ab -n 1000 -c 100 http://localhost:8080/

# Using wrk (better for concurrency)
wrk -t4 -c100 -d30s http://localhost:8080/

# Using siege
siege -c 100 -r 10 http://localhost:8080/
```

### Browser Testing

- Open in Chrome/Firefox/Safari
- Check console for errors
- Test file upload
- Test navigation between pages
- Test images/CSS/JavaScript loading
- Test with developer tools (network tab)

---

## Frontend Integration Points

### API Endpoints Your Backend Must Implement

These are called by the frontend (index.html):

1. **GET /api/test**
   - Called by: "GET" button in HTTP Testing tab
   - Response: 200 OK with any JSON body
   - Example: `{"message": "GET successful"}`

2. **POST /api/test**
   - Called by: "POST" button in HTTP Testing tab
   - Request body: `{"data": "test", "timestamp": "..."}`
   - Response: 200 OK with JSON
   - Example: `{"message": "POST received", "data": "test"}`

3. **DELETE /api/test/1**
   - Called by: "DELETE" button in HTTP Testing tab
   - Response: 200 OK or 204 No Content
   - Example: `{"message": "Resource deleted"}`

4. **POST /upload**
   - Called by: File upload in Upload tab
   - Content-Type: multipart/form-data
   - Response: 200 OK with JSON
   - Example: `{"message": "File uploaded", "filename": "test.jpg"}`

5. **GET /api/metrics** (NEW)
   - Called by: "Fetch Live Metrics" button in Features tab
   - Response: JSON with metrics
   - Example: `{"connections": "10000", "uptime": "99.9%"}`

### Browser Compatibility

Your server must work with:
- Chrome/Chromium
- Firefox
- Safari
- Edge
- Mobile browsers

Test all major features in each browser.

---

## Bonus Features

Implement for extra points (only if mandatory is 100% complete):

### 1. Session Management with Cookies
- Generate unique session IDs
- Send Set-Cookie headers
- Parse Cookie headers from requests
- Store session state
- Implement session timeout

### 2. Multiple CGI Types
- Support PHP, Python, Perl, Shell scripts
- Automatic detection by file extension
- Proper environment setup for each

### 3. Virtual Hosts
- Multiple server blocks on different ports
- Different content per port
- Different error pages per virtual host

### 4. Advanced Features
- WebSocket support
- HTTPS/SSL support
- Authentication/Basic Auth
- CORS headers
- Caching headers (ETag, Last-Modified)

---

## Development Checklist

- [ ] Create Makefile with all targets (all, clean, fclean, re)
- [ ] Parse configuration file correctly
- [ ] Implement non-blocking socket architecture
- [ ] Create listening socket(s) for configured ports
- [ ] Accept client connections
- [ ] Read HTTP requests
- [ ] Parse HTTP headers correctly
- [ ] Implement GET method
- [ ] Implement POST method
- [ ] Implement DELETE method
- [ ] Set up pipe for CGI communication
- [ ] Fork process for CGI execution
- [ ] Set environment variables correctly
- [ ] Execute CGI interpreter with execve
- [ ] Handle CGI output (including chunked)
- [ ] Implement directory listing
- [ ] Implement file uploads to specified directory
- [ ] Implement redirects
- [ ] Return correct HTTP status codes
- [ ] Serve default error pages
- [ ] Handle request timeouts
- [ ] Handle client disconnections
- [ ] Test with curl
- [ ] Test with browser
- [ ] Stress test with 100+ concurrent clients
- [ ] Test with NGINX for comparison

---

## Common Pitfalls to Avoid

1. **Blocking I/O**: Using read/write without poll() first = 0 points
2. **Multiple polls()**: Only 1 poll/select/epoll/kqueue = 0 points
3. **Using fork() for non-CGI**: Only for CGI execution
4. **Memory leaks**: Free all allocated memory
5. **File descriptor leaks**: Close all open fds
6. **Infinite loops**: Implement proper timeouts
7. **Not handling EOF**: Especially for CGI output
8. **Ignoring errno after read/write**: Forbidden by subject
9. **Not handling partial reads/writes**: Must handle both
10. **Not respecting client_max_body_size**: Must reject oversized requests

---

## Resources & References

### Official HTTP Specifications
- **HTTP/1.0 Specification** - https://www.ietf.org/rfc/rfc1945.txt
  - Foundation of web protocols, simpler than 1.1
  - Understanding this helps with 1.1 implementation
  
- **HTTP/1.1 Specification** - https://www.ietf.org/rfc/rfc2616.txt
  - Complete HTTP/1.1 protocol definition
  - Must-read for headers, status codes, methods
  - References for chunked encoding, keep-alive
  
- **CGI Specification (RFC 3875)** - https://tools.ietf.org/html/rfc3875
  - Official CGI standard
  - Environment variable definitions
  - Request/response handling details

### Web Server References

#### NGINX Documentation
- **NGINX Main Docs** - https://nginx.org/en/docs/
  - Configuration directives reference
  - Server behavior comparison
  - Performance tuning examples

#### Apache httpd Documentation
- **Apache Server Docs** - https://httpd.apache.org/docs/
  - Different approach to configuration
  - Useful for understanding alternatives
  
#### IIS Documentation
- **Microsoft IIS** - https://docs.microsoft.com/en-us/iis/
  - Windows server perspective
  - Request handling patterns

### C++ 98 Programming Resources

#### Core Language References
- **C++ Standard Library Reference** - https://en.cppreference.com/
  - C++98 compatible sections
  - All STL containers and algorithms
  - String manipulation, vectors, maps
  
- **GCC/Clang Documentation** - https://gcc.gnu.org/onlinedocs/
  - Compiler-specific features
  - Optimization flags
  - Debugging information

#### Best Practices for C++98
- **Google C++ Style Guide** - https://google.github.io/styleguide/cppguide.html
  - Code organization
  - Naming conventions
  - Memory management
  
- **C++ FAQ Lite** - https://isocpp.org/faq
  - Common C++ questions and answers
  - Best practices (some C++98 specific)
  - Performance tips

### Socket Programming & Networking

#### POSIX Network Programming
- **Beej's Guide to Network Programming** - https://beej.us/guide/bgnet/
  - Excellent introduction to sockets
  - select(), poll() explained clearly
  - Client/server examples
  - Very beginner-friendly
  
- **Linux man pages online** - https://man7.org/linux/man-pages/
  - socket(2), accept(2), listen(2)
  - read(2), write(2), recv(2), send(2)
  - poll(2), select(2), epoll(7)
  - Complete system call reference

#### Advanced I/O Multiplexing
- **select() Reference** - https://man7.org/linux/man-pages/man2/select.2.html
  - Standard method for multiple file descriptors
  - FD_SET macros explanation
  - Timeout handling
  
- **poll() Reference** - https://man7.org/linux/man-pages/man2/poll.2.html
  - More efficient than select()
  - POLLIN, POLLOUT event types
  - Timeout in milliseconds
  
- **epoll Reference** - https://man7.org/linux/man-pages/man7/epoll.7.html
  - Linux-only, highly efficient
  - epoll_create, epoll_ctl, epoll_wait
  - Edge-triggered vs level-triggered
  
- **kqueue Reference** - https://man.freebsd.org/cgi/man.cgi?query=kqueue&sektion=2
  - BSD/macOS alternative to epoll
  - kevent() structures and filters
  - Performance advantages

### Process & System Programming

#### fork() and exec()
- **fork() man page** - https://man7.org/linux/man-pages/man2/fork.2.html
  - Process creation basics
  - Return values and behavior
  
- **execve() man page** - https://man7.org/linux/man-pages/man2/execve.2.html
  - Program execution
  - Argument passing
  - Environment variables

#### Process Management
- **waitpid() man page** - https://man7.org/linux/man-pages/man2/waitpid.2.html
  - Reaping zombie processes
  - Status handling (WIFEXITED, WEXITSTATUS)
  - Proper child cleanup
  
- **pipe() man page** - https://man7.org/linux/man-pages/man2/pipe.2.html
  - Inter-process communication
  - Read/write end handling
  - CGI communication setup

#### File Descriptor Manipulation
- **dup() and dup2() man page** - https://man7.org/linux/man-pages/man2/dup.2.html
  - File descriptor redirection
  - STDIN/STDOUT/STDERR redirection
  - Pipe connection to CGI

### Debugging & Testing Tools

#### Debugging Tools
- **gdb** - GNU Debugger
  - Breakpoints and stepping
  - Variable inspection
  - Stack trace analysis
  - Command: `gdb ./webserv`
  
- **valgrind** - Memory error detector
  - Detect memory leaks
  - Find buffer overflows
  - Command: `valgrind ./webserv`
  
- **strace** - System call tracer
  - See all system calls
  - Debug I/O issues
  - Command: `strace -e trace=network ./webserv`

#### HTTP Testing Tools
- **curl** - Command-line HTTP client
  - Send custom requests
  - Test all HTTP methods
  - Check headers: `curl -i http://localhost:8080/`
  
- **wget** - File download and testing
  - Recursive downloads
  - Mirror entire sites
  - Command: `wget -r http://localhost:8080/`
  
- **telnet** - Raw socket connection
  - Send raw HTTP requests
  - Debug protocol issues
  - Command: `telnet localhost 8080`
  
- **nc (netcat)** - Network utility
  - Port scanning
  - Raw connection testing
  - Command: `nc -l 8080` (listen on port)

#### Load Testing Tools
- **Apache Bench (ab)**
  - Simple concurrent requests
  - Command: `ab -n 1000 -c 100 http://localhost:8080/`
  
- **wrk** - Modern HTTP load tester
  - Better concurrency modeling
  - Lua scripting support
  - Command: `wrk -t4 -c100 -d30s http://localhost:8080/`
  
- **siege** - HTTP load tester
  - URL specification
  - Variable concurrency
  - Command: `siege -c 100 -r 10 http://localhost:8080/`

### File I/O & Directory Operations

#### File Operations
- **stat() man page** - https://man7.org/linux/man-pages/man2/stat.2.html
  - Check file existence and type
  - File permissions
  - File size information
  
- **open() man page** - https://man7.org/linux/man-pages/man2/open.2.html
  - File opening with flags
  - File creation modes
  - Non-blocking file operations

#### Directory Operations
- **opendir()/readdir()/closedir()** - https://man7.org/linux/man-pages/man3/opendir.3.html
  - Directory listing
  - Reading directory entries
  - Directory iteration for listing

#### Path and Access
- **access() man page** - https://man7.org/linux/man-pages/man2/access.2.html
  - Check file permissions (read/write/execute)
  - Verify file accessibility
  
- **chdir() man page** - https://man7.org/linux/man-pages/man2/chdir.2.html
  - Change working directory
  - Important for CGI file access

### String & Data Parsing

#### String Manipulation (C++)
- **std::string Reference** - https://en.cppreference.com/w/cpp/string/basic_string
  - find(), substr(), append(), split patterns
  - Efficient string handling in C++98
  
- **std::map & std::vector** - https://en.cppreference.com/w/cpp/container/
  - Container usage for configuration storage
  - HTTP header storage patterns
  - Memory management

#### Parsing Techniques
- **HTTP Header Parsing** - Understanding state machines
  - Reading until CRLF
  - Header key-value pairs
  - Body content separation

#### Configuration File Parsing
- **NGINX Config** - Study NGINX's own config format
  - Directive syntax
  - Nested blocks
  - Comment handling

### Networking Concepts

#### Understanding Protocols
- **TCP/IP Explained** - https://en.wikipedia.org/wiki/Transmission_Control_Protocol
  - Connection model
  - Three-way handshake
  - Data reliability

- **DNS and getaddrinfo()** - https://man7.org/linux/man-pages/man3/getaddrinfo.3.html
  - Name to IP resolution
  - IPv4 and IPv6 support
  - Address structure manipulation

#### Port and Socket Concepts
- **Port Numbers** - https://en.wikipedia.org/wiki/List_of_TCP_and_UDP_port_numbers
  - Well-known ports (80, 443)
  - Ephemeral ports
  - Port binding restrictions

- **Socket Types** - https://man7.org/linux/man-pages/man2/socket.2.html
  - SOCK_STREAM (TCP)
  - Socket options (SO_REUSEADDR)

### Browser Compatibility & Testing

#### Browser DevTools
- **Chrome DevTools** - F12 in Chrome
  - Network tab for seeing requests
  - Console for JavaScript errors
  - Performance analysis
  
- **Firefox Developer Tools** - F12 in Firefox
  - Similar capabilities
  - Network monitoring
  - Storage inspection

#### Standards & Web APIs
- **MDN Web Docs** - https://developer.mozilla.org/
  - HTTP specification reference
  - Request/response headers
  - Status code meanings

### Performance & Optimization

#### Understanding Performance
- **Big O Notation** - https://en.wikipedia.org/wiki/Big_O_notation
  - Algorithm complexity
  - Why non-blocking I/O matters
  
- **Memory Management in C++** - https://en.cppreference.com/w/cpp/memory
  - Efficient allocation strategies
  - Avoiding fragmentation

### Configuration Management

#### NGINX Configuration Study
```bash
# Study NGINX behavior
nginx -T                    # Print parsed config
nginx -s reload            # Reload config
nginx -s stop              # Stop server
nginx -t                   # Test config syntax
```

#### Configuration Examples
- Create test NGINX config files
- Run NGINX alongside your server
- Compare behavior directly
- Use as reference implementation

### Version Control & Project Management

#### Git
- **Git Tutorial** - https://git-scm.com/docs
  - Track configuration changes
  - Revert broken implementations
  - Team collaboration

#### Testing in Team Environment
- **Unit Testing** - Write tests for components
- **Integration Testing** - Test full flow
- **Regression Testing** - Ensure nothing breaks

### Learning Resources for Team

#### Video Tutorials
- **YouTube: Beej's Guide Videos** - Video version of socket programming guide
- **YouTube: Network Programming** - Various channels covering sockets
- **YouTube: C++ Programming** - General C++ concepts (filter for C++98)

#### Books (Recommended)
- **"Unix Network Programming" by Stevens & Rago**
  - Complete reference for socket programming
  - Real-world examples
  
- **"Effective C++" by Scott Meyers**
  - C++ best practices (some pre-date C++11)
  - Memory management strategies
  
- **"Code Complete" by Steve McConnell**
  - General software construction
  - Debugging and testing practices

### Quick Reference Checklists

#### Socket Programming Checklist
```
1. socket() - Create socket
2. setsockopt() - Set socket options (SO_REUSEADDR)
3. bind() - Bind to address/port
4. listen() - Mark as listening
5. fcntl() - Set non-blocking (O_NONBLOCK)
6. poll/select/epoll/kqueue - Wait for events
7. accept() - Accept connection
8. read/recv - Receive data
9. write/send - Send data
10. close() - Close socket
```

#### HTTP Request Checklist
```
1. Read request line (method, path, version)
2. Parse headers (until blank line)
3. Read body (if Content-Length or chunked)
4. Process request
5. Build response
6. Send response
7. Handle keep-alive or close
```

#### CGI Execution Checklist
```
1. pipe() - Create input/output pipes
2. fork() - Create child process
3. Child: dup2() redirects, setenv(), execve()
4. Parent: write() body, read() output
5. Parent: waitpid() for cleanup
6. Parse output headers/body
7. Send to client
```

### Comparison & Validation Tools

#### NGINX Behavior Comparison
1. **Set up test environment:**
   ```bash
   mkdir test_server
   cd test_server
   ```

2. **Create config for NGINX:**
   ```nginx
   server {
       listen 9090;
       root /var/www/html;
       location / {
           allow_methods GET POST DELETE;
       }
   }
   ```

3. **Run both servers:**
   ```bash
   nginx -c /path/to/nginx.conf
   ./webserv config.conf
   ```

4. **Compare responses:**
   ```bash
   curl -v http://localhost:8080/test
   curl -v http://localhost:9090/test
   ```

### Community & Getting Help

#### Stack Overflow Tags
- Search for: `c++ sockets`, `http server`, `socket programming`
- Filter for C++98 relevant answers

#### 42 School Resources
- **42 Wiki** - Share notes and solutions (within academic integrity rules)
- **Discord/Slack** - Student communities for 42
- **Pair Programming** - Learn by teaching teammates

### Recommended Study Order

1. **Week 1:** HTTP protocol, read RFC 1945 & 2616
2. **Week 2:** Socket programming, study Beej's guide
3. **Week 3:** Multiplexing (poll/select/epoll/kqueue)
4. **Week 4:** Process management (fork, exec, pipe)
5. **Week 5:** CGI implementation
6. **Week 6:** Configuration parsing
7. **Week 7:** Integration & testing
8. **Week 8:** Optimization & stress testing
9. **Week 9:** Bonus features
10. **Week 10:** Final testing & debugging

---

## Contact & Questions

For unclear behavior, compare with NGINX:
1. Set up same route in NGINX
2. Check response headers
3. Check status codes
4. Match behavior exactly

When stuck:
1. Check man pages: `man socket`, `man poll`, `man fork`
2. Use strace to see what's happening: `strace -f ./webserv`
3. Use gdb to debug: `gdb ./webserv`
4. Compare with NGINX behavior
5. Read RFC specifications
6. Ask teammates and instructors

Good luck with your implementation! 🚀
