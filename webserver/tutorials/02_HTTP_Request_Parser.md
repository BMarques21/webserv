# Tutorial 02: HTTP Request Parser

## 📖 What You'll Learn

- Why we need a request parser
- How to parse incrementally (for non-blocking I/O)
- Parsing the request line
- Parsing headers
- Parsing the body
- Error handling and validation

---

## 🎯 Why Do We Need a Parser?

### The Problem:
```
Raw bytes from socket:
"GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n"

We need:
- Method: GET
- URI: /index.html
- Version: HTTP/1.1
- Headers: { "host": "localhost" }
```

### The Solution:
A **parser** that converts raw bytes into structured data.

---

## 🏗️ Parser Architecture

### State Machine Approach

```
┌──────────────┐
│REQUEST_LINE  │──────> Parse "GET /path HTTP/1.1"
└──────┬───────┘
       │ Found \r\n
       ↓
┌──────────────┐
│   HEADERS    │──────> Parse "Key: Value" pairs
└──────┬───────┘
       │ Found \r\n\r\n (empty line)
       ↓
┌──────────────┐
│     BODY     │──────> Read Content-Length bytes
└──────┬───────┘
       │ All bytes read
       ↓
┌──────────────┐
│   COMPLETE   │──────> Request ready!
└──────────────┘
```

### Why a State Machine?

1. **Incremental Parsing** - Can handle partial data
2. **Non-Blocking** - Works with poll()/select()
3. **Clear Transitions** - Easy to understand and debug
4. **Resumable** - Can pause and continue later

---

## 📝 Implementation Details

### Class Structure

```cpp
enum ParseState {
    REQUEST_LINE,  // Parsing "GET /path HTTP/1.1"
    HEADERS,       // Parsing headers
    BODY,          // Reading body
    COMPLETE,      // Done!
    ERROR          // Something wrong
};

class HttpRequest {
private:
    ParseState state;           // Current parsing state
    std::string raw_data;       // Accumulated data
    size_t bytes_parsed;        // How many bytes processed
    
    HttpMethod method;          // GET, POST, DELETE
    std::string uri;            // "/index.html"
    std::string query_string;   // "?param=value"
    std::string http_version;   // "HTTP/1.1"
    
    std::map<std::string, std::string> headers;
    std::string body;
    size_t content_length;
    
public:
    bool parse(const char* data, size_t len);  // Main parsing function
};
```

### Why This Design?

- **state**: Tracks where we are in parsing
- **raw_data**: Accumulates incoming bytes
- **bytes_parsed**: Remembers our position
- **Separate fields**: Clean data structure after parsing

---

## 🔍 Parsing the Request Line

### Goal:
```
Input:  "GET /index.html?user=john HTTP/1.1\r\n"
Output: method=GET, uri="/index.html", query="user=john", version="HTTP/1.1"
```

### Implementation:

```cpp
void HttpRequest::parseRequestLine(const std::string& line) {
    std::istringstream iss(line);
    std::string method_str;
    
    // Split by spaces: "GET /path HTTP/1.1"
    iss >> method_str >> uri >> http_version;
    
    // Convert string to enum
    method = stringToMethod(method_str);
    
    // Validate method
    if (method == UNKNOWN) {
        error_code = 405;  // Method Not Allowed
        state = ERROR;
        return;
    }
    
    // Check HTTP version
    if (http_version != "HTTP/1.1" && http_version != "HTTP/1.0") {
        error_code = 505;  // HTTP Version Not Supported
        state = ERROR;
        return;
    }
    
    // Extract query string
    parseQueryString();
    
    state = HEADERS;  // Move to next state
}
```

### Why Split Query String?

```
URI: "/search?q=webserver&lang=cpp"
       ↓
path: "/search"
query: "q=webserver&lang=cpp"
```

Benefits:
- Easier to route based on path
- Query parameters separate from routing
- Matches common web framework patterns

### Implementation:

```cpp
void HttpRequest::parseQueryString() {
    size_t pos = uri.find('?');
    if (pos != std::string::npos) {
        query_string = uri.substr(pos + 1);  // After '?'
        uri = uri.substr(0, pos);            // Before '?'
    }
}
```

---

## 📋 Parsing Headers

### Goal:
```
Input:  "Host: localhost\r\n"
        "Content-Length: 123\r\n"
        "\r\n"  ← Empty line = headers done

Output: headers["host"] = "localhost"
        headers["content-length"] = "123"
```

### Why Lowercase Keys?

```cpp
// HTTP headers are case-insensitive
"Content-Type" == "content-type" == "CONTENT-TYPE"

// We normalize to lowercase
headers[toLower(key)] = value;

// Now lookup is easy:
std::string ct = getHeader("Content-Type");  // Works!
std::string ct = getHeader("content-type");  // Also works!
```

### Implementation:

```cpp
void HttpRequest::parseHeader(const std::string& line) {
    // Find the colon separator
    size_t pos = line.find(':');
    if (pos == std::string::npos) {
        error_code = 400;  // Bad Request
        state = ERROR;
        return;
    }
    
    // Split: "Key: Value" → "Key" and "Value"
    std::string key = trim(line.substr(0, pos));
    std::string value = trim(line.substr(pos + 1));
    
    // Store with lowercase key
    headers[toLower(key)] = value;
}
```

### Special Headers:

```cpp
// After all headers parsed:
if (line.empty()) {
    // Get Content-Length for body parsing
    std::string content_len_str = getHeader("Content-Length");
    if (!content_len_str.empty()) {
        content_length = parseNumber(content_len_str);
    }
    
    // Extract boundary for multipart/form-data
    std::string content_type = getHeader("Content-Type");
    if (content_type.find("multipart/form-data") != std::string::npos) {
        extractBoundary(content_type);
    }
    
    // Decide next state
    if (content_length > 0) {
        state = BODY;
    } else {
        state = COMPLETE;
    }
}
```

---

## 📦 Parsing the Body

### Two Approaches:

#### 1. **Content-Length** (We use this)
```
Content-Length: 27

name=john&email=john@example.com
↑                           ↑
byte 0                   byte 26

Read exactly 27 bytes, then done!
```

#### 2. **Chunked Transfer** (Not implemented)
```
5\r\n          ← Chunk size in hex (5 bytes)
Hello\r\n      ← Chunk data
5\r\n
World\r\n
0\r\n          ← Final chunk (size 0)
\r\n
```

### Our Implementation (Content-Length):

```cpp
if (state == BODY) {
    size_t available = raw_data.size() - bytes_parsed;
    
    if (available >= content_length) {
        // We have all the body!
        body = raw_data.substr(bytes_parsed, content_length);
        bytes_parsed += content_length;
        state = COMPLETE;
    } else {
        // Need more data, wait for next parse() call
        return false;
    }
}
```

### Why This Works with Non-Blocking I/O:

```
Call 1: recv() returns 100 bytes
        → parse() → need 200 bytes → return false (not complete)

Call 2: recv() returns 100 more bytes  
        → parse() → have 200 bytes now → COMPLETE!
```

---

## 🔄 The Main Parse Function

### How It Works:

```cpp
bool HttpRequest::parse(const char* data, size_t len) {
    // Append new data to buffer
    raw_data.append(data, len);
    
    // Process until complete or need more data
    while (state != COMPLETE && state != ERROR) {
        
        if (state == REQUEST_LINE || state == HEADERS) {
            // Look for line ending
            size_t line_end = raw_data.find("\r\n", bytes_parsed);
            
            if (line_end == std::string::npos) {
                // No complete line yet, need more data
                return false;
            }
            
            // Extract line
            std::string line = raw_data.substr(bytes_parsed, 
                                             line_end - bytes_parsed);
            bytes_parsed = line_end + 2;  // Skip \r\n
            
            if (state == REQUEST_LINE) {
                parseRequestLine(line);
            } else {
                if (line.empty()) {
                    // Empty line = end of headers
                    transitionToBody();
                } else {
                    parseHeader(line);
                }
            }
            
        } else if (state == BODY) {
            // Read based on Content-Length
            parseBody();
        }
    }
    
    return state == COMPLETE;
}
```

### Key Points:

1. **Accumulate data**: `raw_data.append(data, len)`
2. **Find delimiters**: `find("\r\n")`
3. **Track position**: `bytes_parsed`
4. **Process incrementally**: Return false if need more data
5. **Loop until done**: Keep processing if we have enough data

---

## ⚠️ Error Handling

### Security Limits:

```cpp
// Prevent DoS attacks
if (raw_data.size() > 8192) {  // 8KB limit
    error_code = 431;  // Request Header Fields Too Large
    state = ERROR;
}

// Check content length
if (content_length > max_body_size) {
    error_code = 413;  // Payload Too Large
    state = ERROR;
}
```

### Validation:

```cpp
// Invalid HTTP method
if (method == UNKNOWN) {
    error_code = 405;  // Method Not Allowed
}

// Malformed request line
if (uri.empty() || http_version.empty()) {
    error_code = 400;  // Bad Request
}

// Unsupported HTTP version
if (http_version != "HTTP/1.1") {
    error_code = 505;  // HTTP Version Not Supported
}
```

---

## 💡 Design Decisions

### Why Map for Headers?

```cpp
std::map<std::string, std::string> headers;
```

**Pros:**
- Fast lookup: O(log n)
- Sorted keys (nice for debugging)
- Standard C++98 container

**Alternatives:**
- Array: Slower lookup
- Hash map: Not in C++98
- Vector of pairs: O(n) lookup

### Why String for Method?

```cpp
enum HttpMethod { GET, POST, DELETE };
```

**Benefits:**
- Type-safe
- Fast comparison (integer)
- Compiler checks validity
- Easy switch statements

**vs. String:**
```cpp
if (method_str == "GET")  // String comparison (slower)
if (method == GET)        // Integer comparison (faster)
```

### Why Incremental Parsing?

**Without (Blocking):**
```cpp
// Wait for ALL data before parsing
std::string full_request = readAll(socket);  // BLOCKS!
parse(full_request);
```

**With (Non-Blocking):**
```cpp
// Parse as data arrives
char buf[1024];
ssize_t n = recv(socket, buf, 1024, 0);  // Returns immediately
if (n > 0) {
    request.parse(buf, n);  // Parse partial data
}
```

---

## 🎯 Real-World Example

### Scenario: User uploads a 10MB file

```
Iteration 1: recv() returns 4096 bytes
  → parse() → state=REQUEST_LINE
  → Parses "POST /upload HTTP/1.1"
  → state=HEADERS
  
Iteration 2: recv() returns 4096 bytes
  → parse() → state=HEADERS
  → Parses all headers
  → Content-Length: 10485760  (10MB!)
  → state=BODY
  
Iterations 3-2563: recv() returns chunks
  → parse() → state=BODY
  → Accumulates body data
  → bytes_read < content_length → return false
  
Iteration 2564: Final chunk received
  → parse() → state=BODY
  → bytes_read == content_length
  → state=COMPLETE → return true
```

**Benefits:**
- Server doesn't block waiting for 10MB
- Can handle other clients while uploading
- Memory efficient (can stream to disk)

---

## 🔍 Common Questions for Defense

**Q: Why use a state machine?**
- Clear states and transitions
- Easy to debug (know exactly where we are)
- Handles partial data naturally
- Standard pattern for parsers

**Q: What if we receive data in middle of a line?**
```
Receive 1: "GET /index"
Receive 2: ".html HTTP/1.1\r\n"

Our parser:
- Accumulates: "GET /index.html HTTP/1.1\r\n"
- Finds \r\n at end
- Parses complete line
```

**Q: Why lowercase header keys?**
- HTTP spec says headers are case-insensitive
- Normalizing makes lookup easier
- Prevents duplicate keys with different cases

**Q: What about malformed requests?**
- We validate each step
- Set appropriate error codes (400, 405, 431, 505)
- State transitions to ERROR
- Caller checks `isValid()` and `getErrorCode()`

**Q: How do you handle multiple requests on same connection?**
- Call `reset()` after processing request
- Clears all data, resets state to REQUEST_LINE
- Ready for next request (keep-alive)

---

## 📚 Code References

### Files:
- `includes/HttpRequest.hpp` - Class definition
- `srcs/HttpRequest.cpp` - Implementation

### Key Functions:
```cpp
bool parse(const char* data, size_t len);    // Main entry point
void parseRequestLine(const std::string&);    // Parse first line
void parseHeader(const std::string&);         // Parse one header
void parseQueryString();                      // Extract query params
HttpMethod stringToMethod(const std::string&); // Convert to enum
```

---

## 🧪 Testing Your Understanding

### Exercise 1: Trace This Request
```http
POST /api/login HTTP/1.1
Host: localhost
Content-Length: 29

username=john&password=secret
```

**Questions:**
1. What state after first parse()?
2. What's stored in `uri`?
3. What's stored in `headers["content-length"]`?
4. When does state become COMPLETE?

### Exercise 2: Find the Error
```http
PATCH /resource HTTP/1.1
Host: localhost
```

**What error code and why?**

### Exercise 3: Incomplete Data
```cpp
char buf1[] = "GET /ind";
request.parse(buf1, 8);  // What's the state?

char buf2[] = "ex.html HTTP/1.1\r\n\r\n";
request.parse(buf2, 20); // What's the state now?
```

---

**Continue to [Tutorial 03: HTTP Response Generator](03_HTTP_Response_Generator.md) →**
