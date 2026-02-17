# Tutorial 07: Architecture & Design Patterns

## 📖 What You'll Learn

- Component separation rationale
- Design patterns used
- C++98 constraints and impacts
- Error handling strategy
- Testing philosophy
- Code organization principles

---

## 🎯 Overall Architecture

### High-Level View

```
┌─────────────────────────────────────────────────────┐
│                   WebServer                         │
│  - Event loop (poll)                               │
│  - Connection management                           │
│  - Request routing                                 │
└─────────────┬───────────────────────────────────────┘
              │
              ├─────────────────────────────────────────┐
              ↓                                         ↓
┌─────────────────────┐                   ┌─────────────────────┐
│   HttpRequest       │                   │   HttpResponse      │
│  - State machine    │                   │  - Response builder │
│  - Incremental parse│                   │  - Status codes     │
└─────────────────────┘                   └─────────────────────┘
              │                                         ↑
              │                                         │
              ↓                                         │
┌─────────────────────────────────────────────────────┐│
│              Request Handlers                       ││
│  ┌──────────────────┐  ┌──────────────────┐        ││
│  │ StaticFileHandler│  │  UploadHandler   │        ││
│  │ - Serve files    │  │ - Parse multipart│        ││
│  │ - MIME detection │  │ - Save uploads   │        ││
│  └──────────────────┘  └──────────────────┘        ││
└─────────────────────────────────────────────────────┘│
                                                       │
                                                       │
              ┌────────────────────────────────────────┘
              │
              ↓
        [HTTP Response]
```

### Component Responsibilities

| Component | Responsibility | Knows About |
|-----------|---------------|-------------|
| **WebServer** | Event loop, connections | Request, Response, Handlers |
| **HttpRequest** | Parse HTTP requests | Nothing (standalone) |
| **HttpResponse** | Build HTTP responses | Nothing (standalone) |
| **StaticFileHandler** | Serve files | Request, Response |
| **UploadHandler** | Handle uploads | Request, Response |

---

## 🏛️ Design Patterns Used

### 1. State Machine Pattern

**Used in:** HttpRequest parser

**Problem:** HTTP requests arrive in chunks
**Solution:** Track parsing state, process incrementally

```cpp
enum ParseState {
    REQUEST_LINE,    // Parsing "GET /path HTTP/1.1"
    HEADERS,         // Parsing "Key: Value"
    BODY,            // Reading body content
    COMPLETE         // Request fully parsed
};
```

**Benefits:**
- Clear state transitions
- Easy to debug (print current state)
- Handles partial data
- No blocking

**Pattern in action:**
```cpp
bool HttpRequest::parse(const std::string& chunk) {
    buffer += chunk;
    
    switch (state) {
        case REQUEST_LINE:
            if (parseRequestLine()) state = HEADERS;
            break;
        case HEADERS:
            if (parseHeaders()) state = BODY;
            break;
        case BODY:
            if (parseBody()) state = COMPLETE;
            break;
        case COMPLETE:
            return true;
    }
    
    return state == COMPLETE;
}
```

### 2. Factory Pattern (Static Methods)

**Used in:** HttpResponse helpers

**Problem:** Creating common responses is verbose
**Solution:** Static factory methods

```cpp
// Without factory (verbose):
HttpResponse resp(404);
resp.setContentType("text/html");
resp.setBody("<h1>404 Not Found</h1>");

// With factory (clean):
HttpResponse resp = HttpResponse::notFound();
```

**Implementation:**
```cpp
class HttpResponse {
public:
    static HttpResponse ok(const std::string& content);
    static HttpResponse notFound(const std::string& msg = "");
    static HttpResponse redirect(const std::string& location);
    // ... etc
};
```

**Benefits:**
- Readable code
- Less duplication
- Consistent responses
- Easy to modify

### 3. Handler Pattern

**Used in:** Request routing

**Problem:** Different requests need different processing
**Solution:** Separate handlers for different tasks

```cpp
HttpResponse routeRequest(const HttpRequest& request) {
    if (request.getMethod() == "GET") {
        StaticFileHandler handler("/var/www");
        return handler.handle(request);
    }
    else if (request.getMethod() == "POST") {
        UploadHandler handler("uploads", 10 * 1024 * 1024);
        return handler.handle(request);
    }
    else {
        return HttpResponse::badRequest("Method not supported");
    }
}
```

**Benefits:**
- Separation of concerns
- Easy to add new handlers
- Testable in isolation
- Clear responsibilities

### 4. Builder Pattern

**Used in:** HttpResponse construction

**Problem:** Response has many parts (status, headers, body)
**Solution:** Build incrementally, assemble at end

```cpp
HttpResponse response(200);
response.setContentType("text/html");
response.setHeader("Cache-Control", "no-cache");
response.setBody("<h1>Hello</h1>");

std::string http_response = response.build();  // Assemble everything
```

**Benefits:**
- Flexible construction
- Can modify before building
- Automatic Content-Length
- Clear separation (build vs. set)

---

## 🔧 C++98 Constraints

### What We Can't Use

| Feature | Available in | Alternative |
|---------|-------------|-------------|
| `auto` keyword | C++11 | Explicit types |
| Lambda functions | C++11 | Regular functions |
| `nullptr` | C++11 | `NULL` or `0` |
| Range-based for | C++11 | Iterator loops |
| `std::to_string()` | C++11 | `std::ostringstream` |
| Smart pointers | C++11 | Raw pointers (careful!) |
| `std::unordered_map` | C++11 | `std::map` |
| `.front()` / `.back()` | C++11 | `[0]` / `[size()-1]` |

### Impact on Code

#### 1. No auto keyword

```cpp
// C++11:
auto it = headers.find("Content-Type");

// C++98:
std::map<std::string, std::string>::iterator it = headers.find("Content-Type");
```

#### 2. No range-based for

```cpp
// C++11:
for (const auto& header : headers) {
    std::cout << header.first << ": " << header.second << std::endl;
}

// C++98:
for (std::map<std::string, std::string>::const_iterator it = headers.begin();
     it != headers.end(); ++it) {
    std::cout << it->first << ": " << it->second << std::endl;
}
```

#### 3. No std::to_string()

```cpp
// C++11:
std::string str = std::to_string(200);

// C++98:
std::ostringstream oss;
oss << 200;
std::string str = oss.str();
```

#### 4. No nullptr

```cpp
// C++11:
char* ptr = nullptr;

// C++98:
char* ptr = NULL;  // or 0
```

### Why These Constraints?

**Project requirement:** Must work with older compilers
- Teaches fundamentals (not modern C++ shortcuts)
- More verbose but explicit
- Good for understanding

---

## ⚠️ Error Handling Strategy

### Design Philosophy

**Principle:** Fail gracefully, inform user

1. **Detect errors early**
2. **Return meaningful responses**
3. **Log for debugging**
4. **Never crash server**

### Levels of Error Handling

#### 1. Protocol Errors (Client Fault)

**Examples:**
- Malformed request
- Missing required headers
- Invalid method

**Response:** 4xx status codes

```cpp
if (request.getMethod().empty()) {
    return HttpResponse::badRequest("Invalid request line");
}

if (content_type.find("multipart/form-data") == std::string::npos) {
    return HttpResponse::badRequest("Invalid Content-Type for upload");
}
```

#### 2. Resource Errors (Client Request)

**Examples:**
- File not found
- Permission denied
- File too large

**Response:** 4xx status codes

```cpp
if (stat(full_path.c_str(), &st) != 0) {
    return HttpResponse::notFound("File not found: " + uri);
}

if (file_size > max_file_size) {
    return HttpResponse(413).setBody("File too large");
}
```

#### 3. Server Errors (Our Fault)

**Examples:**
- Can't open file (despite existing)
- Disk full
- Internal parsing error

**Response:** 5xx status codes

```cpp
if (!saveFile(path, content)) {
    return HttpResponse::internalServerError("Failed to save file");
}
```

### Error Handling Pattern

```cpp
HttpResponse handle(const HttpRequest& request) {
    // Validation
    if (invalid_input) {
        return HttpResponse::badRequest("...");
    }
    
    // Processing
    try {
        Result result = process();
        if (!result.success) {
            return HttpResponse::internalServerError("...");
        }
    } catch (...) {
        // C++98: exceptions are risky
        return HttpResponse::internalServerError("Unexpected error");
    }
    
    // Success
    return HttpResponse::ok("...");
}
```

### What We Don't Do

**1. No throwing exceptions across boundaries**
```cpp
// Bad:
HttpResponse handler() {
    throw std::runtime_error("Error!");  // Who catches this?
}

// Good:
HttpResponse handler() {
    return HttpResponse::internalServerError("Error!");
}
```

**2. No silent failures**
```cpp
// Bad:
if (!saveFile(path, content)) {
    // Ignore error
}

// Good:
if (!saveFile(path, content)) {
    return HttpResponse::internalServerError("Save failed");
}
```

**3. No assert() in production**
```cpp
// Bad:
assert(request.getMethod() == "POST");  // Crashes server!

// Good:
if (request.getMethod() != "POST") {
    return HttpResponse::badRequest("Expected POST");
}
```

---

## 🧪 Testing Strategy

### Unit Testing Philosophy

**What to test:**
1. Parsing logic (state machine)
2. Response generation
3. MIME type detection
4. Filename sanitization
5. Boundary extraction

**How to test:**
```cpp
void test_request_parsing() {
    HttpRequest req;
    
    // Test 1: Simple GET
    bool complete = req.parse("GET /index.html HTTP/1.1\r\n\r\n");
    assert(complete == true);
    assert(req.getMethod() == "GET");
    assert(req.getUri() == "/index.html");
    
    // Test 2: Incremental parsing
    HttpRequest req2;
    assert(req2.parse("GET /") == false);  // Incomplete
    assert(req2.parse("index.html HTTP/1.1\r\n\r\n") == true);  // Complete
}
```

### Integration Testing

**What to test:**
1. Full request/response cycle
2. File upload and retrieval
3. Error responses
4. Multiple clients (stress test)

**Example:**
```python
# test_integration.py
import requests

# Upload file
files = {'file': open('test.jpg', 'rb')}
response = requests.post('http://localhost:8080/upload', files=files)
assert response.status_code == 201

# Retrieve file
response = requests.get('http://localhost:8080/uploads/test.jpg')
assert response.status_code == 200
assert response.headers['Content-Type'] == 'image/jpeg'
```

### Manual Testing

**Tools:**
- curl
- Browser
- Postman

**Commands:**
```bash
# GET request
curl http://localhost:8080/index.html

# POST upload
curl -F "file=@photo.jpg" http://localhost:8080/upload

# DELETE
curl -X DELETE http://localhost:8080/uploads/photo.jpg

# Headers
curl -I http://localhost:8080/index.html
```

---

## 📁 Code Organization

### Directory Structure

```
webserver/
├── includes/               # Header files
│   ├── HttpRequest.hpp
│   ├── HttpResponse.hpp
│   ├── StaticFileHandler.hpp
│   └── UploadHandler.hpp
├── srcs/                   # Implementation files
│   ├── HttpRequest.cpp
│   ├── HttpResponse.cpp
│   ├── StaticFileHandler.cpp
│   └── UploadHandler.cpp
├── tests/                  # Test programs
│   ├── test_http.cpp
│   ├── test_parser.py
│   └── test_upload.py
├── www/                    # Static files
│   ├── index.html
│   └── upload.html
├── uploads/                # Upload destination
├── docs/                   # Documentation
│   ├── API.md
│   └── IMPLEMENTATION.md
├── tutorials/              # Learning materials
│   ├── 01_HTTP_Protocol_Basics.md
│   ├── 02_HTTP_Request_Parser.md
│   └── ...
├── Makefile
└── README.md
```

### Why This Structure?

**includes/ and srcs/ separation:**
- Clear interface vs. implementation
- Easier to navigate
- Standard C++ convention

**tests/ directory:**
- All tests in one place
- Easy to run all tests
- Separate from production code

**www/ for static files:**
- Simulates real web server
- Test static file serving
- Example HTML forms

**tutorials/ for learning:**
- Educational materials
- Explains theory and practice
- Helps with project defense

### File Naming Conventions

**Headers:** `ClassName.hpp`
**Implementation:** `ClassName.cpp`
**Tests:** `test_feature.cpp` or `test_feature.py`

**Why:**
- Easy to find corresponding files
- Consistent and predictable
- Standard C++ practice

---

## 💡 Key Design Decisions

### 1. Why Separate Request and Response?

**Decision:** HttpRequest and HttpResponse are independent classes

**Alternatives:**
```cpp
// Alternative: Combined
class HttpTransaction {
    HttpRequest request;
    HttpResponse response;
};
```

**Our choice is better because:**
- Request parsing is independent
- Response generation is flexible
- Can test separately
- Clearer responsibilities

### 2. Why String for Body?

**Decision:** `std::string body` in HttpRequest/Response

**Alternatives:**
- `std::vector<char>` for binary data
- `char*` with manual memory management

**Our choice:**
- std::string handles memory automatically
- Works for both text and binary (C++98)
- Easy to manipulate
- Size tracking built-in

**Trade-off:**
- Not ideal for multi-GB files
- But we limit file size anyway (10MB)

### 3. Why Map for Headers?

**Decision:** `std::map<std::string, std::string> headers`

**Benefits:**
- Fast lookup: O(log n)
- Sorted (consistent output)
- No duplicates
- C++98 compatible

**Trade-off:**
- HTTP allows duplicate headers (rare)
- Last value wins in our implementation
- Good enough for webserver use case

### 4. Why Static Handlers?

**Decision:** Handlers don't maintain state between requests

```cpp
// Create new handler per request
StaticFileHandler handler("/var/www");
HttpResponse resp = handler.handle(request);
```

**Alternative:** Singleton handler
```cpp
// Share one handler
static StaticFileHandler& getHandler() {
    static StaticFileHandler handler("/var/www");
    return handler;
}
```

**Our choice is better because:**
- No shared state (thread-safe ready)
- Easy to test
- Different configs per request
- Simpler to understand

---

## 🔍 Real-World Parallels

### Similar to nginx/Apache?

**Yes, conceptually:**
- Event-driven (poll/epoll)
- Modular handlers
- Non-blocking I/O

**Differences:**
- They use epoll (faster than poll)
- Thread pools for CPU work
- Advanced caching
- Complex config language
- Production-grade error handling

**Our project:**
- Educational (understand concepts)
- Simpler (easier to explain)
- Full-featured (handles real requests)
- Extensible (can add features)

---

## 🔍 Common Questions for Defense

**Q: Why not use existing HTTP library (libcurl, etc.)?**
- This is a learning project
- Understanding HTTP from scratch
- Know exactly how it works
- Can customize for specific needs

**Q: How would you make this production-ready?**
1. Add epoll (Linux) or kqueue (BSD) for scalability
2. Thread pool for CPU-intensive tasks
3. Connection pooling and keep-alive
4. Logging framework (not cout)
5. Configuration file parsing
6. HTTPS support (TLS/SSL)
7. Rate limiting and DDoS protection
8. Comprehensive error recovery

**Q: What's the biggest bottleneck?**
- File I/O (reading large files)
- Solution: sendfile() syscall (zero-copy)
- Or: cache frequently accessed files

**Q: How to handle CGI?**
- Fork child process
- Set environment variables (REQUEST_METHOD, etc.)
- Exec CGI script
- Read stdout → send as response
- Challenge: Non-blocking I/O with pipes

**Q: Why C++98 and not modern C++?**
- Project requirement (compatibility)
- Teaches fundamentals
- More verbose = more explicit
- Good for understanding

**Q: How to add virtual hosts?**
```cpp
// Parse Host header
std::string host = request.getHeader("Host");

// Route based on host
if (host == "example.com") {
    StaticFileHandler handler("/var/www/example");
    return handler.handle(request);
}
else if (host == "test.com") {
    StaticFileHandler handler("/var/www/test");
    return handler.handle(request);
}
```

---

## 📚 Code Quality Principles

### 1. Single Responsibility

Each class has one job:
- HttpRequest: Parse requests
- HttpResponse: Build responses
- StaticFileHandler: Serve files
- UploadHandler: Handle uploads

### 2. Don't Repeat Yourself (DRY)

```cpp
// Bad: Duplicate error pages
if (error1) return HttpResponse(404).setBody("<h1>404</h1>");
if (error2) return HttpResponse(404).setBody("<h1>404</h1>");

// Good: Reuse
if (error1 || error2) return HttpResponse::notFound();
```

### 3. Clear Naming

```cpp
// Bad:
bool p(const std::string& s);

// Good:
bool parse(const std::string& chunk);
```

### 4. Const Correctness

```cpp
std::string getMethod() const;  // Doesn't modify object
std::string getHeader(const std::string& key) const;
```

### 5. Error Messages Matter

```cpp
// Bad:
return HttpResponse::badRequest("Error");

// Good:
return HttpResponse::badRequest("Missing Content-Type header for upload");
```

---

## 🧪 Testing Your Understanding

### Exercise 1: Design Patterns
Match pattern to use case:
1. State Machine
2. Factory
3. Builder
4. Handler

a) Creating common HTTP responses
b) Incremental request parsing
c) Routing requests to different processors
d) Constructing HTTP response with many parts

### Exercise 2: C++98 Conversion
Convert to C++98:
```cpp
// C++11
auto headers = request.getHeaders();
for (const auto& [key, value] : headers) {
    std::cout << key << ": " << value << "\n";
}
```

### Exercise 3: Error Handling
What status code for:
1. File doesn't exist?
2. Upload exceeds size limit?
3. Can't save file (disk error)?
4. Invalid HTTP method?

---

**Continue to [Tutorial 08: Advanced Topics & Extensions](08_Advanced_Topics.md) →**
