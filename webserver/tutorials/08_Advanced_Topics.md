# Tutorial 08: Advanced Topics & Extensions

## 📖 What You'll Learn

- CGI (Common Gateway Interface) execution
- Configuration file parsing
- Chunked transfer encoding
- HTTP keep-alive connections
- Virtual hosts
- HTTPS/TLS basics
- Performance optimizations
- Future enhancements

---

## 🎯 Beyond the Basics

This tutorial covers advanced features that could extend your webserver. While not all are implemented in the current version, understanding them is valuable for project defense and future development.

---

## 🔧 CGI (Common Gateway Interface)

### What is CGI?

**Purpose:** Execute programs to generate dynamic content

**Example:**
```
Client requests: GET /cgi-bin/hello.py
Server executes: /var/www/cgi-bin/hello.py
Script outputs: HTTP headers + HTML
Server sends output to client
```

### How CGI Works

```
Client Request
    ↓
Server receives: GET /cgi-bin/script.py?name=John
    ↓
Server forks child process
    ↓
Sets environment variables:
  - REQUEST_METHOD=GET
  - QUERY_STRING=name=John
  - CONTENT_TYPE=...
  - CONTENT_LENGTH=...
    ↓
Executes script: python script.py
    ↓
Reads script's stdout
    ↓
Sends to client as HTTP response
```

### Implementation Outline

```cpp
HttpResponse executeCGI(const HttpRequest& request, 
                       const std::string& script_path) {
    // 1. Create pipe for reading output
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        return HttpResponse::internalServerError("Pipe creation failed");
    }
    
    // 2. Fork process
    pid_t pid = fork();
    
    if (pid < 0) {
        return HttpResponse::internalServerError("Fork failed");
    }
    
    if (pid == 0) {
        // Child process
        
        // Redirect stdout to pipe
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        
        // Set environment variables
        setenv("REQUEST_METHOD", request.getMethod().c_str(), 1);
        setenv("QUERY_STRING", request.getQueryString().c_str(), 1);
        setenv("CONTENT_TYPE", request.getHeader("Content-Type").c_str(), 1);
        
        // Execute script
        execl("/usr/bin/python", "python", script_path.c_str(), NULL);
        
        // If execl returns, error occurred
        exit(1);
    }
    else {
        // Parent process
        
        close(pipefd[1]);  // Close write end
        
        // Read script output
        std::string output;
        char buffer[4096];
        ssize_t bytes;
        
        while ((bytes = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            output.append(buffer, bytes);
        }
        
        close(pipefd[0]);
        
        // Wait for child
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // Success - output contains headers + body
            return HttpResponse::ok(output);
        }
        else {
            return HttpResponse::internalServerError("CGI script failed");
        }
    }
}
```

### CGI Script Example

**hello.py:**
```python
#!/usr/bin/env python
import os

# Read environment variables
method = os.environ.get('REQUEST_METHOD', 'GET')
query = os.environ.get('QUERY_STRING', '')

# Parse query string
params = {}
if query:
    for pair in query.split('&'):
        key, value = pair.split('=')
        params[key] = value

name = params.get('name', 'World')

# Output HTTP headers
print("Content-Type: text/html")
print()  # Empty line separates headers from body

# Output body
print("<html><body>")
print(f"<h1>Hello, {name}!</h1>")
print(f"<p>Request method: {method}</p>")
print("</body></html>")
```

### CGI Environment Variables

| Variable | Example | Meaning |
|----------|---------|---------|
| `REQUEST_METHOD` | `GET` | HTTP method |
| `QUERY_STRING` | `name=John&age=25` | URL parameters |
| `CONTENT_TYPE` | `application/json` | Request body type |
| `CONTENT_LENGTH` | `1234` | Request body size |
| `PATH_INFO` | `/extra/path` | Extra path after script |
| `SCRIPT_NAME` | `/cgi-bin/hello.py` | Script being executed |
| `SERVER_NAME` | `localhost` | Server hostname |
| `SERVER_PORT` | `8080` | Server port |

### Challenges

**1. Non-blocking I/O**
- CGI blocks while script runs
- Use separate thread or process pool
- Or: FastCGI (persistent processes)

**2. Security**
- Validate script path (no `../../`)
- Limit execution time (timeout)
- Sanitize environment variables
- Use chroot jail

**3. Performance**
- Fork + exec is slow
- Alternative: FastCGI (reuse processes)
- Or: Embedded interpreters (mod_python style)

---

## 📄 Configuration File Parsing

### Why Config Files?

**Hard-coded (Bad):**
```cpp
StaticFileHandler handler("/var/www");  // Fixed path
UploadHandler uploader("uploads", 10 * 1024 * 1024);  // Fixed size
```

**Configurable (Good):**
```cpp
Config config("webserver.conf");
StaticFileHandler handler(config.get("static_root"));
UploadHandler uploader(config.get("upload_dir"), 
                      config.getInt("max_upload_size"));
```

### Simple Config Format

**webserver.conf:**
```
server_name = localhost
port = 8080
root = /var/www
upload_dir = /var/www/uploads
max_upload_size = 10485760  # 10MB
index = index.html index.htm
error_page_404 = /errors/404.html

# Virtual host
[virtualhost example.com]
root = /var/www/example
index = index.html
```

### Parser Implementation

```cpp
class Config {
private:
    std::map<std::string, std::string> values;
    
public:
    bool load(const std::string& filename) {
        std::ifstream file(filename.c_str());
        if (!file.is_open()) return false;
        
        std::string line;
        std::string section;
        
        while (std::getline(file, line)) {
            // Remove comments
            size_t comment = line.find('#');
            if (comment != std::string::npos) {
                line = line.substr(0, comment);
            }
            
            // Trim whitespace
            line = trim(line);
            
            // Skip empty lines
            if (line.empty()) continue;
            
            // Section header: [section_name]
            if (line[0] == '[' && line[line.length()-1] == ']') {
                section = line.substr(1, line.length() - 2);
                continue;
            }
            
            // Key = value
            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                std::string key = trim(line.substr(0, eq));
                std::string value = trim(line.substr(eq + 1));
                
                // Prefix with section if in section
                if (!section.empty()) {
                    key = section + "." + key;
                }
                
                values[key] = value;
            }
        }
        
        return true;
    }
    
    std::string get(const std::string& key, 
                   const std::string& default_value = "") const {
        std::map<std::string, std::string>::const_iterator it = values.find(key);
        return (it != values.end()) ? it->second : default_value;
    }
    
    int getInt(const std::string& key, int default_value = 0) const {
        std::string value = get(key);
        if (value.empty()) return default_value;
        return atoi(value.c_str());
    }
};
```

### Usage Example

```cpp
Config config;
if (!config.load("webserver.conf")) {
    std::cerr << "Failed to load config" << std::endl;
    return 1;
}

int port = config.getInt("port", 8080);
std::string root = config.get("root", "/var/www");
int max_upload = config.getInt("max_upload_size", 10 * 1024 * 1024);

// Virtual host config
std::string vh_root = config.get("virtualhost example.com.root");
```

---

## 📦 Chunked Transfer Encoding

### The Problem

**Normal transfer:**
```http
HTTP/1.1 200 OK
Content-Length: 1234

[exactly 1234 bytes of data]
```

**Problem:** What if size is unknown?
- Dynamically generated content
- Real-time streaming
- Don't want to buffer entire response

### The Solution

**Chunked encoding:**
```http
HTTP/1.1 200 OK
Transfer-Encoding: chunked

5\r\n
Hello\r\n
6\r\n
 World\r\n
0\r\n
\r\n
```

**Format:**
```
[size in hex]\r\n
[data of that size]\r\n
[size in hex]\r\n
[data]\r\n
...
0\r\n
\r\n
```

### Sending Chunked Response

```cpp
HttpResponse sendChunked(const std::string& data) {
    std::ostringstream response;
    
    // Status line
    response << "HTTP/1.1 200 OK\r\n";
    
    // Headers
    response << "Transfer-Encoding: chunked\r\n";
    response << "\r\n";
    
    // Chunks (send 1KB at a time)
    size_t chunk_size = 1024;
    for (size_t i = 0; i < data.length(); i += chunk_size) {
        size_t len = std::min(chunk_size, data.length() - i);
        
        // Chunk size (hex)
        response << std::hex << len << "\r\n";
        
        // Chunk data
        response << data.substr(i, len) << "\r\n";
    }
    
    // Final chunk (size 0)
    response << "0\r\n\r\n";
    
    return HttpResponse::raw(response.str());
}
```

### Receiving Chunked Request

```cpp
std::string parseChunkedBody(const std::string& body) {
    std::string result;
    size_t pos = 0;
    
    while (pos < body.length()) {
        // Read chunk size (hex number until \r\n)
        size_t line_end = body.find("\r\n", pos);
        if (line_end == std::string::npos) break;
        
        std::string size_str = body.substr(pos, line_end - pos);
        size_t chunk_size = strtol(size_str.c_str(), NULL, 16);
        
        if (chunk_size == 0) {
            // Last chunk
            break;
        }
        
        pos = line_end + 2;  // Skip \r\n
        
        // Read chunk data
        result.append(body.substr(pos, chunk_size));
        
        pos += chunk_size + 2;  // Skip data + \r\n
    }
    
    return result;
}
```

---

## 🔄 HTTP Keep-Alive

### The Problem

**Without keep-alive:**
```
1. Client connects
2. Client sends request
3. Server sends response
4. Connection closes
5. Client connects again (new request)
   - TCP handshake overhead
   - Slow!
```

**With keep-alive:**
```
1. Client connects
2. Client sends request 1
3. Server sends response 1
4. Client sends request 2 (same connection!)
5. Server sends response 2
6. Connection stays open
```

### Implementation

**Server side:**
```cpp
bool shouldKeepAlive(const HttpRequest& request) {
    std::string connection = request.getHeader("Connection");
    
    // HTTP/1.1 default is keep-alive
    if (request.getVersion() == "HTTP/1.1") {
        return connection != "close";
    }
    
    // HTTP/1.0 needs explicit Connection: keep-alive
    return connection == "keep-alive";
}

void handleClient(int client_fd) {
    bool keep_alive = true;
    
    while (keep_alive) {
        // Read request
        HttpRequest request;
        // ... parse ...
        
        // Process
        HttpResponse response = processRequest(request);
        
        // Decide keep-alive
        keep_alive = shouldKeepAlive(request);
        
        if (keep_alive) {
            response.setHeader("Connection", "keep-alive");
            response.setHeader("Keep-Alive", "timeout=5, max=100");
        } else {
            response.setHeader("Connection", "close");
        }
        
        // Send response
        send(client_fd, response.build());
        
        if (!keep_alive) {
            break;
        }
        
        // Reset for next request
        request = HttpRequest();
    }
    
    close(client_fd);
}
```

### Challenges

**Timeout:**
- Close connection if idle too long
- Use poll() timeout

```cpp
// Poll with 5 second timeout
int ready = poll(fds, num_fds, 5000);
if (ready == 0) {
    // Timeout - close idle connections
    for (each connection) {
        if (time_since_last_activity > 5) {
            close(connection);
        }
    }
}
```

**Max requests:**
- Limit requests per connection
- Prevent resource exhaustion

```cpp
struct ClientConnection {
    int requests_served;
    const int max_requests = 100;
    
    bool shouldClose() {
        return requests_served >= max_requests;
    }
};
```

---

## 🌐 Virtual Hosts

### Concept

**One server, multiple websites:**
```
example.com → /var/www/example
test.com    → /var/www/test
blog.com    → /var/www/blog
```

**All on same IP:port, distinguished by `Host` header:**
```http
GET / HTTP/1.1
Host: example.com
```

### Implementation

```cpp
class VirtualHostRouter {
private:
    std::map<std::string, std::string> host_roots;
    std::string default_root;
    
public:
    VirtualHostRouter(const std::string& default_root) 
        : default_root(default_root) {}
    
    void addVirtualHost(const std::string& host, 
                       const std::string& root) {
        host_roots[host] = root;
    }
    
    std::string getRoot(const HttpRequest& request) {
        std::string host = request.getHeader("Host");
        
        // Remove port if present (example.com:8080 → example.com)
        size_t colon = host.find(':');
        if (colon != std::string::npos) {
            host = host.substr(0, colon);
        }
        
        // Lookup virtual host
        std::map<std::string, std::string>::iterator it = host_roots.find(host);
        if (it != host_roots.end()) {
            return it->second;
        }
        
        return default_root;
    }
};

// Usage:
VirtualHostRouter router("/var/www/default");
router.addVirtualHost("example.com", "/var/www/example");
router.addVirtualHost("test.com", "/var/www/test");

HttpResponse handle(const HttpRequest& request) {
    std::string root = router.getRoot(request);
    StaticFileHandler handler(root);
    return handler.handle(request);
}
```

---

## 🔒 HTTPS / TLS Basics

### Why HTTPS?

**HTTP (Insecure):**
- Data sent in plain text
- Anyone can read/modify
- Passwords visible!

**HTTPS (Secure):**
- Data encrypted with TLS
- Certificate validates server identity
- Protects privacy and integrity

### TLS Handshake (Simplified)

```
1. Client → Server: Hello (supported ciphers)
2. Server → Client: Certificate + chosen cipher
3. Client verifies certificate
4. Client + Server: Exchange keys
5. Encrypted communication begins
```

### Implementation (Requires OpenSSL)

**Not implemented in our basic server, but outline:**

```cpp
#include <openssl/ssl.h>
#include <openssl/err.h>

class HTTPSServer {
private:
    SSL_CTX* ssl_ctx;
    
public:
    HTTPSServer() {
        // Initialize OpenSSL
        SSL_load_error_strings();
        OpenSSL_add_ssl_algorithms();
        
        // Create SSL context
        ssl_ctx = SSL_CTX_new(TLS_server_method());
        
        // Load certificate and private key
        SSL_CTX_use_certificate_file(ssl_ctx, "cert.pem", SSL_FILETYPE_PEM);
        SSL_CTX_use_PrivateKey_file(ssl_ctx, "key.pem", SSL_FILETYPE_PEM);
    }
    
    void handleClient(int client_fd) {
        // Create SSL structure
        SSL* ssl = SSL_new(ssl_ctx);
        SSL_set_fd(ssl, client_fd);
        
        // TLS handshake
        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            return;
        }
        
        // Read encrypted request
        char buffer[4096];
        int bytes = SSL_read(ssl, buffer, sizeof(buffer));
        
        // ... parse request ...
        
        // Send encrypted response
        std::string response = "HTTP/1.1 200 OK\r\n...";
        SSL_write(ssl, response.c_str(), response.length());
        
        // Cleanup
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
};
```

**Note:** Full HTTPS implementation is complex and beyond this project scope.

---

## ⚡ Performance Optimizations

### 1. File Caching

**Problem:** Reading same file repeatedly

**Solution:** Cache in memory

```cpp
class FileCache {
private:
    struct CachedFile {
        std::string content;
        time_t last_modified;
    };
    
    std::map<std::string, CachedFile> cache;
    
public:
    std::string getFile(const std::string& path) {
        // Check cache
        std::map<std::string, CachedFile>::iterator it = cache.find(path);
        
        // Get file modification time
        struct stat st;
        stat(path.c_str(), &st);
        
        // Cache hit and not modified?
        if (it != cache.end() && it->second.last_modified == st.st_mtime) {
            return it->second.content;
        }
        
        // Read from disk
        std::string content = readFile(path);
        
        // Update cache
        CachedFile cf;
        cf.content = content;
        cf.last_modified = st.st_mtime;
        cache[path] = cf;
        
        return content;
    }
};
```

### 2. sendfile() System Call

**Problem:** read() + send() = two copies

```
Disk → Kernel buffer → User space buffer → Kernel buffer → Network
       [read()]        [copy to app]        [send()]
```

**Solution:** sendfile() = zero-copy

```
Disk → Kernel buffer → Network
       [sendfile()]
```

**Implementation:**
```cpp
#include <sys/sendfile.h>

void sendFileEfficient(int socket_fd, const std::string& path) {
    int file_fd = open(path.c_str(), O_RDONLY);
    
    struct stat st;
    fstat(file_fd, &st);
    off_t size = st.st_size;
    
    // Send HTTP headers first
    std::string headers = "HTTP/1.1 200 OK\r\n"
                         "Content-Length: " + toString(size) + "\r\n"
                         "\r\n";
    send(socket_fd, headers.c_str(), headers.length(), 0);
    
    // Send file with zero-copy
    off_t offset = 0;
    sendfile(socket_fd, file_fd, &offset, size);
    
    close(file_fd);
}
```

### 3. Connection Pooling

**Reuse connections to databases or backend services**

### 4. Compression

**Gzip response bodies:**
```http
HTTP/1.1 200 OK
Content-Encoding: gzip
Content-Length: 234

[compressed data]
```

**Requires:** zlib library

---

## 🚀 Future Enhancements

### 1. WebSocket Support

**Real-time bidirectional communication**

```http
GET /chat HTTP/1.1
Upgrade: websocket
Connection: Upgrade
```

### 2. HTTP/2

**Features:**
- Multiplexing (multiple requests on one connection)
- Server push
- Header compression (HPACK)
- Binary protocol (not text)

### 3. Rate Limiting

**Prevent abuse:**
```cpp
class RateLimiter {
private:
    std::map<std::string, int> request_counts;  // IP → count
    std::map<std::string, time_t> last_reset;   // IP → timestamp
    
public:
    bool allowRequest(const std::string& client_ip) {
        time_t now = time(NULL);
        
        // Reset counter every minute
        if (now - last_reset[client_ip] > 60) {
            request_counts[client_ip] = 0;
            last_reset[client_ip] = now;
        }
        
        // Check limit (100 requests per minute)
        if (request_counts[client_ip] >= 100) {
            return false;  // Rate limit exceeded
        }
        
        request_counts[client_ip]++;
        return true;
    }
};
```

### 4. Authentication

**Basic Auth:**
```http
Authorization: Basic dXNlcjpwYXNz  (base64 encoded "user:pass")
```

**Implementation:**
```cpp
bool checkAuth(const HttpRequest& request) {
    std::string auth = request.getHeader("Authorization");
    
    if (auth.find("Basic ") != 0) {
        return false;
    }
    
    std::string encoded = auth.substr(6);  // Skip "Basic "
    std::string decoded = base64_decode(encoded);
    
    // decoded = "username:password"
    size_t colon = decoded.find(':');
    std::string username = decoded.substr(0, colon);
    std::string password = decoded.substr(colon + 1);
    
    // Check credentials
    return (username == "admin" && password == "secret");
}
```

### 5. Logging

**Access log:**
```
127.0.0.1 - - [10/Jan/2024:14:30:45 +0000] "GET /index.html HTTP/1.1" 200 1234
```

**Error log:**
```
[2024-01-10 14:30:45] [ERROR] Failed to open file: /var/www/missing.html
```

---

## 🔍 Common Questions for Defense

**Q: How would you add HTTPS support?**
1. Integrate OpenSSL library
2. Load certificate and private key
3. Wrap socket with SSL_accept()
4. Use SSL_read/SSL_write instead of recv/send
5. Handle certificate validation errors

**Q: What's the biggest performance bottleneck?**
- File I/O (disk reads)
- Solution: caching + sendfile()

**Q: How to handle millions of connections?**
1. Use epoll (Linux) or kqueue (BSD) instead of poll
2. Thread pool for CPU work
3. Connection limits and timeouts
4. Horizontal scaling (load balancer + multiple servers)

**Q: Security concerns?**
1. Path traversal (fixed with sanitization)
2. File size limits (implemented)
3. DoS attacks (need rate limiting)
4. Code injection (validate CGI inputs)
5. HTTPS for encryption

**Q: Why not use an existing web framework?**
- Learning project (understand from scratch)
- Full control over implementation
- Performance tuning possible
- Educational value

---

## 📚 Further Learning

### Books
- "HTTP: The Definitive Guide" - Comprehensive HTTP reference
- "Unix Network Programming" - Socket programming
- "The Linux Programming Interface" - System calls

### RFCs (Standards)
- RFC 7230-7235: HTTP/1.1 specification
- RFC 2616: HTTP/1.1 (older version)
- RFC 3875: CGI specification
- RFC 6455: WebSocket protocol

### Tools
- `tcpdump` / `wireshark` - Network packet analysis
- `ab` (Apache Bench) - Load testing
- `curl` - HTTP client testing
- `strace` - System call tracing

---

## 🎓 Project Defense Tips

### What to Emphasize

1. **Understanding, not just code:**
   - Explain WHY, not just WHAT
   - "We use poll() because it doesn't have the 1024 FD limit of select()"

2. **Trade-offs:**
   - "We chose X over Y because..."
   - "This works for our use case, but in production we'd use..."

3. **Limitations:**
   - Be honest about what it can't do
   - Shows you understand scope

4. **Real-world parallels:**
   - "Like nginx, we use event-driven architecture"
   - "Similar to Apache's CGI implementation"

### Questions to Prepare For

- How does your parser handle malformed input?
- What happens if a client disconnects mid-upload?
- How do you prevent path traversal attacks?
- Why use poll() instead of threads?
- What's the maximum file size your server can handle?
- How would you add feature X?
- What are the security vulnerabilities?
- How does your server compare to nginx?

### Demonstration Ideas

1. **Show it working:**
   - Upload a file
   - Serve static content
   - Handle multiple clients

2. **Show error handling:**
   - Request non-existent file (404)
   - Upload too-large file (413)
   - Send malformed request (400)

3. **Show code:**
   - Walk through state machine
   - Explain key design decision
   - Show a complex function and explain it

---

## 🏁 Conclusion

You now have a comprehensive understanding of:

✅ HTTP protocol fundamentals
✅ Request parsing (state machine)
✅ Response generation
✅ Static file serving
✅ File upload handling
✅ Non-blocking I/O with poll()
✅ Architecture and design patterns
✅ Advanced topics and extensions

**Your webserver can:**
- Handle multiple clients simultaneously
- Parse HTTP requests incrementally
- Serve static files with correct MIME types
- Accept file uploads (multipart/form-data)
- Generate proper HTTP responses
- Handle errors gracefully

**You can explain:**
- Why each design decision was made
- How the components work together
- What the limitations are
- How to extend it further

**Good luck with your project defense!** 🚀

---

**← Back to [Tutorial Index](00_INDEX.md)**
