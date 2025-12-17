# Webserv - HTTP/1.1 Server in C++98

A lightweight HTTP/1.1 web server implementation in C++98, created as part of the 42 school curriculum.

## Features

### Adapted from ft_irc:
- ✅ **Non-blocking I/O with poll()** - Efficient event-driven architecture
- ✅ **Socket management** - Setup, binding, listening, accepting connections
- ✅ **Client management** - Connection tracking with buffers and state
- ✅ **Output buffering** - Non-blocking write with POLLOUT events
- ✅ **Timeout handling** - Automatic cleanup of inactive clients
- ✅ **Makefile structure** - Progress bar, proper flags, clean build system

### HTTP Server Specific:
- ✅ GET method (fully functional)
- ✅ Basic error handling (404, 500)
- ✅ Static file serving
- ✅ Content-Type detection
- 🔄 POST, DELETE methods
- 🔄 Configuration file parsing (basic structure ready)
- 🔄 CGI execution (framework ready, needs testing)
- 🔄 File uploads
- 🔄 Directory listing (autoindex)
- 🔄 Multiple server blocks
- 🔄 Virtual hosts support
- 🔄 Custom error pages (structure ready)

## Project Structure

```
webserv/
├── Makefile                   # Build system with progress bar
├── README.md                  # This file
├── .gitignore                 # Git ignore rules
│
├── config/
│   └── webserv.conf          # Server configuration (NGINX-style)
│
├── inc/                       # Header files
│   ├── Server.hpp            # Main server class (adapted from ft_irc)
│   ├── Client.hpp            # Client connection management
│   ├── Request.hpp           # HTTP request parser
│   ├── Response.hpp          # HTTP response builder
│   ├── Config.hpp            # Configuration file parser
│   ├── CgiHandler.hpp        # CGI script execution
│   ├── HttpStatus.hpp        # HTTP status codes and messages
│   └── Utils.hpp             # Utility functions (file I/O, string ops)
│
├── src/                       # Source files
│   ├── server/               # Core server implementation
│   │   ├── main.cpp          # Entry point
│   │   ├── Server.cpp        # Poll loop, socket handling
│   │   ├── Client.cpp        # Client state management
│   │   ├── Request.cpp       # HTTP request parsing
│   │   ├── Response.cpp      # HTTP response building
│   │   └── Config.cpp        # Configuration parsing
│   ├── CgiHandler.cpp        # CGI execution (fork/exec/pipes)
│   ├── HttpStatus.cpp        # Status code mappings
│   └── Utils.cpp             # Helper functions
│
├── www/                       # Document root
│   ├── index.html            # Default index page
│   ├── 404.html              # 404 error page
│   └── 500.html              # 500 error page
│
├── tests/                     # Testing utilities
│   └── run_tests.sh          # Automated test script
│
└── docs/                      # Documentation
    ├── IMPLEMENTATION_PLAN.md    # Development roadmap
    └── ADAPTATION_NOTES.md       # ft_irc → webserv notes
```

## Compilation

```bash
make
```

## Usage

```bash
./webserv [config_file]
```

Default config: `config/webserv.conf`

## Testing

### Automated Tests
```bash
# Run all tests
./tests/run_tests.sh
```

### Manual Testing
```bash
# Start the server
./webserv

# In another terminal, test with curl:
curl http://localhost:8080/
curl -i http://localhost:8080/nonexistent.html

# Multiple requests stress test
for i in {1..100}; do curl -s -o /dev/null http://localhost:8080/; done

# Or test with a browser:
firefox http://localhost:8080/

# Test with telnet (manual HTTP):
telnet localhost 8080
GET / HTTP/1.1
Host: localhost
[press Enter twice]
```

### Expected Results
- ✅ GET /: HTTP 200 OK with HTML content
- ✅ GET /nonexistent: HTTP 404 Not Found
- ✅ Multiple requests: All succeed without crashes
- ✅ Content-Type: Correct MIME types
- ✅ Server stability: No crashes or hangs

## Key Adaptations from ft_irc

### 1. Poll-based Event Loop
The core `poll()` loop structure was directly adapted:
- Monitor multiple file descriptors (server socket + client connections)
- Handle POLLIN (incoming data) and POLLOUT (ready to write) events
- Non-blocking operations throughout

### 2. Client Buffer Management
Similar buffering strategy for incomplete requests:
- Accumulate data in client buffer until complete request received
- Parse when delimiters found (IRC: `\r\n`, HTTP: `\r\n\r\n` for headers)
- Handle partial data gracefully

### 3. Output Buffering
Reused the output buffer mechanism:
- Queue response data in output buffer
- Set POLLOUT flag when data pending
- Write when socket ready, handle EAGAIN/EWOULDBLOCK
- Remove POLLOUT when buffer empty

### 4. Socket Setup
Standard socket creation, binding, listening code:
- `SO_REUSEADDR` for quick restarts
- Non-blocking mode with `fcntl()`
- Error handling patterns

## TODO

### High Priority (Mandatory)
- [ ] Complete configuration file parsing (NGINX-style)
- [ ] Implement POST method with body handling
- [ ] Implement DELETE method
- [ ] Add file upload functionality
- [ ] Implement directory listing (autoindex)
- [ ] Add CGI support with non-blocking I/O
- [ ] Support multiple server blocks
- [ ] Implement chunked transfer encoding
- [ ] Request body size limits (max_body_size)

### Medium Priority (Enhancements)
- [ ] Custom error pages from configuration
- [ ] HTTP redirections (301/302)
- [ ] Keep-alive connection support
- [ ] Multiple CGI interpreters
- [ ] Virtual host support
- [ ] Range requests

### Bonus Features
- [ ] Cookie and session management
- [ ] Multiple CGI types with examples
- [ ] WebSocket support (if time permits)

### Testing & Quality
- [ ] Stress testing with siege/ab
- [ ] Memory leak testing with valgrind
- [ ] Browser compatibility testing
- [ ] Compare with NGINX behavior
- [ ] Comprehensive error scenario testing

## Documentation

- **[IMPLEMENTATION_PLAN.md](docs/IMPLEMENTATION_PLAN.md)** - Detailed development roadmap with phases
- **[ADAPTATION_NOTES.md](docs/ADAPTATION_NOTES.md)** - Technical details on ft_irc adaptations
- **[QUICKSTART.md](QUICKSTART.md)** - Quick start guide for immediate use

## Compliance

- **C++ 98 standard**
- **Allowed functions**: execve, dup, dup2, pipe, fork, socketpair, htons, htonl, ntohs, ntohl, select, poll, epoll, kqueue, socket, accept, listen, send, recv, bind, connect, getaddrinfo, freeaddrinfo, setsockopt, getsockname, getprotobyname, fcntl, close, read, write, waitpid, kill, signal, access, stat, open, opendir, readdir, closedir
- **No external libraries** (except standard C++98)

## Authors

viceda-s

