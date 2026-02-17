# Tutorial 03: HTTP Response Generator

## 📖 What You'll Learn

- How to build HTTP responses
- Status codes and when to use them
- Header management
- Content-Length calculation
- Default error pages
- Helper methods pattern

---

## 🎯 The Goal

Transform this:
```cpp
HttpResponse response(200);
response.setBody("<h1>Hello</h1>");
```

Into this:
```http
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 16
Server: WebServ/1.0

<h1>Hello</h1>
```

---

## 🏗️ Response Structure

### Class Design

```cpp
class HttpResponse {
private:
    int status_code;              // 200, 404, 500, etc.
    std::string status_message;   // "OK", "Not Found", etc.
    std::map<std::string, std::string> headers;
    std::string body;
    
public:
    HttpResponse(int code = 200);
    void setStatusCode(int code);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& content);
    std::string build();  // Creates the complete response string
    
    // Helper methods
    static HttpResponse ok(const std::string& content);
    static HttpResponse notFound();
    static HttpResponse redirect(const std::string& location);
};
```

### Why This Design?

1. **Separate status and message** - Status is int (fast), message is readable
2. **Map for headers** - Easy to add/modify headers
3. **build() method** - Assembles everything at the end
4. **Static helpers** - Quick creation of common responses

---

## 🔢 Status Codes Mapping

### The Translation Table

```cpp
std::string HttpResponse::getStatusMessage(int code) const {
    switch (code) {
        // 2xx - Success
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        
        // 3xx - Redirection
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        
        // 4xx - Client Errors
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        
        // 5xx - Server Errors
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 503: return "Service Unavailable";
        
        default: return "Unknown";
    }
}
```

### Why Separate?

```cpp
// Easy to set
response.setStatusCode(404);  // Just a number

// Easy to get message
std::string msg = response.getStatusMessage(404);  // "Not Found"

// Clean in status line
"HTTP/1.1 " + code + " " + message  // "HTTP/1.1 404 Not Found"
```

---

## 📋 Header Management

### Setting Headers

```cpp
void HttpResponse::setHeader(const std::string& key, 
                            const std::string& value) {
    headers[key] = value;
}

// Usage:
response.setHeader("Content-Type", "text/html");
response.setHeader("Cache-Control", "max-age=3600");
response.setHeader("X-Custom-Header", "my-value");
```

### Default Headers

```cpp
HttpResponse::HttpResponse(int code) {
    status_code = code;
    status_message = getStatusMessage(code);
    
    // Always add Server header
    setHeader("Server", "WebServ/1.0");
}
```

**Why always add Server?**
- Identifies your software
- Standard practice
- Helps with debugging
- Can be used for stats/analytics

### Content-Type Header

```cpp
void HttpResponse::setContentType(const std::string& mime_type) {
    setHeader("Content-Type", mime_type);
}

// Usage:
response.setContentType("text/html");
response.setContentType("application/json");
response.setContentType("image/png");
```

---

## 📏 Content-Length Calculation

### Automatic Calculation

```cpp
void HttpResponse::setBody(const std::string& content) {
    body = content;
    
    // Automatically set Content-Length
    std::ostringstream oss;
    oss << content.size();
    setHeader("Content-Length", oss.str());
}
```

### Why Automatic?

**Manual (Error-Prone):**
```cpp
response.setBody("<h1>Hello</h1>");
response.setHeader("Content-Length", "15");  // Oops! Should be 16
```

**Automatic (Safe):**
```cpp
response.setBody("<h1>Hello</h1>");  // Content-Length: 16 (automatic!)
```

### Counting Bytes

```cpp
std::string content = "<h1>Hello</h1>";
size_t bytes = content.size();  // 16 bytes

// Count: < h 1 > H e l l o < / h 1 >
//        1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6
```

**Important:** We count *bytes*, not characters!
- ASCII: 1 char = 1 byte
- UTF-8: 1 char can be 1-4 bytes

---

## 🏗️ Building the Response

### The build() Method

```cpp
std::string HttpResponse::build() {
    std::ostringstream response;
    
    // 1. Status line
    response << "HTTP/1.1 " << status_code << " " 
             << status_message << "\r\n";
    
    // 2. Headers
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        response << it->first << ": " << it->second << "\r\n";
    }
    
    // 3. Empty line (separates headers from body)
    response << "\r\n";
    
    // 4. Body
    response << body;
    
    return response.str();
}
```

### Step-by-Step Example

**Input:**
```cpp
HttpResponse resp(200);
resp.setContentType("text/html");
resp.setBody("<h1>Hi</h1>");
```

**Output of build():**
```http
HTTP/1.1 200 OK\r\n
Content-Length: 11\r\n
Content-Type: text/html\r\n
Server: WebServ/1.0\r\n
\r\n
<h1>Hi</h1>
```

**Key Points:**
1. Status line first
2. Headers (order doesn't matter)
3. Empty line (`\r\n\r\n`)
4. Body

---

## 🎨 Helper Methods Pattern

### The Problem
```cpp
// Without helpers (verbose):
HttpResponse resp(404);
resp.setContentType("text/html");
std::string body = "<html><body><h1>404 Not Found</h1></body></html>";
resp.setBody(body);
```

### The Solution
```cpp
// With helpers (clean):
HttpResponse resp = HttpResponse::notFound();
```

### Implementation

```cpp
// Static helper - returns ready-to-use response
static HttpResponse HttpResponse::notFound(const std::string& message) {
    HttpResponse response(404);
    
    std::string body = "<html><body>"
                      "<h1>404 Not Found</h1>"
                      "<p>" + message + "</p>"
                      "</body></html>";
    
    response.setContentType("text/html");
    response.setBody(body);
    
    return response;
}
```

### Common Helpers

#### 1. **200 OK**
```cpp
static HttpResponse ok(const std::string& content, 
                      const std::string& content_type = "text/html") {
    HttpResponse response(200);
    response.setContentType(content_type);
    response.setBody(content);
    return response;
}

// Usage:
return HttpResponse::ok("<h1>Success!</h1>");
return HttpResponse::ok("{\"status\": \"ok\"}", "application/json");
```

#### 2. **404 Not Found**
```cpp
static HttpResponse notFound(const std::string& message = "Not Found") {
    HttpResponse response(404);
    std::string body = "<html><body><h1>404 Not Found</h1>"
                      "<p>" + message + "</p></body></html>";
    response.setBody(body);
    response.setContentType("text/html");
    return response;
}

// Usage:
return HttpResponse::notFound();
return HttpResponse::notFound("The page you requested does not exist");
```

#### 3. **302 Redirect**
```cpp
static HttpResponse redirect(const std::string& location, int code = 302) {
    HttpResponse response(code);
    response.setHeader("Location", location);
    
    std::string body = "<html><body>"
                      "<h1>Redirect</h1>"
                      "<p>Redirecting to <a href=\"" + location + "\">" 
                      + location + "</a></p>"
                      "</body></html>";
    
    response.setBody(body);
    response.setContentType("text/html");
    return response;
}

// Usage:
return HttpResponse::redirect("/new-page");
return HttpResponse::redirect("/moved", 301);  // Permanent
```

#### 4. **500 Internal Server Error**
```cpp
static HttpResponse internalServerError(const std::string& message = "") {
    HttpResponse response(500);
    std::string body = "<html><body>"
                      "<h1>500 Internal Server Error</h1>"
                      "<p>" + message + "</p>"
                      "</body></html>";
    response.setBody(body);
    response.setContentType("text/html");
    return response;
}

// Usage:
try {
    // ... risky operation
} catch (...) {
    return HttpResponse::internalServerError("Database connection failed");
}
```

---

## 🎨 Default Error Pages

### Why Custom Error Pages?

**Default (Ugly):**
```
404 Not Found
nginx/1.18.0
```

**Custom (Nice):**
```html
<!DOCTYPE html>
<html>
<head><title>404 Not Found</title></head>
<body>
  <h1>404 - Page Not Found</h1>
  <p>The requested resource could not be found.</p>
  <a href="/">Go to homepage</a>
</body>
</html>
```

### Implementation Strategy

```cpp
class HttpResponse {
    static std::string buildErrorPage(int code, 
                                      const std::string& message) {
        std::string title;
        std::string description;
        
        switch (code) {
            case 400:
                title = "Bad Request";
                description = "The request could not be understood.";
                break;
            case 404:
                title = "Not Found";
                description = "The requested resource was not found.";
                break;
            // ... etc
        }
        
        std::ostringstream html;
        html << "<html><head><title>" << code << " " << title 
             << "</title></head>"
             << "<body style='font-family: Arial; text-align: center; "
             << "padding-top: 50px;'>"
             << "<h1>" << code << " " << title << "</h1>"
             << "<p>" << description << "</p>"
             << "<p>" << message << "</p>"
             << "</body></html>";
        
        return html.str();
    }
};
```

---

## 💡 Design Decisions

### Why Return by Value?

```cpp
static HttpResponse ok(const std::string& content) {
    HttpResponse response(200);
    // ...
    return response;  // Copy!
}
```

**Concern:** Isn't this slow (copying)?

**Answer:** No, because of **RVO** (Return Value Optimization)
- Compiler optimizes away the copy
- In C++98, compilers already do this
- Cleaner code is worth it

**Alternative (Pointer - Messy):**
```cpp
static HttpResponse* ok(...) {
    HttpResponse* response = new HttpResponse(200);
    return response;  // Caller must delete!
}
```

### Why Map for Headers?

```cpp
std::map<std::string, std::string> headers;
```

**Benefits:**
- Sorted (consistent order in output)
- Fast lookup
- No duplicates
- C++98 compatible

**Trade-offs:**
- HTTP allows duplicate headers (rare)
- We only keep last value
- Good enough for our use case

### Why Not Use cout?

```cpp
// Don't do this:
void HttpResponse::send(int socket) {
    std::cout << "HTTP/1.1 " << status_code << "\r\n";
    // ...
}
```

**Problems:**
- Couples response to output
- Can't test easily
- Can't modify before sending
- Less flexible

**Better:**
```cpp
std::string response_str = response.build();
send(socket, response_str.c_str(), response_str.length(), 0);
```

---

## 🔍 Real-World Usage

### Example 1: Serving a File

```cpp
std::string content = readFile("/path/to/file.html");
if (content.empty()) {
    return HttpResponse::notFound("File does not exist");
}

HttpResponse response = HttpResponse::ok(content, "text/html");
return response;
```

### Example 2: Handling Upload

```cpp
if (uploaded) {
    HttpResponse response(201);  // Created
    response.setHeader("Location", "/uploads/" + filename);
    response.setBody("<h1>Upload Successful</h1>");
    response.setContentType("text/html");
    return response;
} else {
    return HttpResponse::internalServerError("Failed to save file");
}
```

### Example 3: API Endpoint

```cpp
HttpResponse response = HttpResponse::ok(
    "{\"users\": [\"john\", \"jane\"]}", 
    "application/json"
);
return response;
```

---

## 🔍 Common Questions for Defense

**Q: Why separate setBody() and build()?**
- **setBody()**: Sets content, calculates Content-Length
- **build()**: Assembles everything (can be called multiple times)
- Separation of concerns

**Q: What if I set Content-Length manually?**
```cpp
response.setBody("Hello");        // Content-Length: 5
response.setHeader("Content-Length", "10");  // Overrides to 10 (wrong!)
```
- Last write wins
- Better to rely on automatic calculation

**Q: Can you add headers after setBody()?**
```cpp
response.setBody("Hello");
response.setHeader("Cache-Control", "no-cache");  // Yes, this works!
```
- Order doesn't matter for headers
- build() assembles them all at the end

**Q: Why include body in error responses?**
- Better user experience (shows what went wrong)
- Browsers display the HTML
- Debugging easier
- Standards compliant

**Q: What's the difference between 301 and 302?**
- **301**: Permanent redirect (update bookmarks)
- **302**: Temporary redirect (original URL still valid)
- Search engines treat them differently

---

## 📚 Code References

### Files:
- `includes/HttpResponse.hpp` - Class definition
- `srcs/HttpResponse.cpp` - Implementation

### Key Methods:
```cpp
HttpResponse(int code);                    // Constructor
void setStatusCode(int code);              // Change status
void setHeader(const std::string& k, v);   // Add header
void setBody(const std::string& content);  // Set body + Content-Length
std::string build();                       // Build complete response

// Static helpers
static HttpResponse ok(...);
static HttpResponse notFound(...);
static HttpResponse redirect(...);
static HttpResponse badRequest(...);
static HttpResponse internalServerError(...);
```

---

## 🧪 Testing Your Understanding

### Exercise 1: Build a Response
Create a response for successful login:
- Status: 200
- Set a cookie
- JSON body: `{"success": true, "token": "abc123"}`

### Exercise 2: Error Handling
What response would you send if:
1. User tries to upload 100MB file (limit is 10MB)?
2. User requests a file that doesn't exist?
3. User sends malformed JSON in POST?

### Exercise 3: Headers Order
Does this change the response?
```cpp
// Version A
response.setContentType("text/html");
response.setBody("<h1>Hi</h1>");

// Version B
response.setBody("<h1>Hi</h1>");
response.setContentType("text/html");
```

---

**Continue to [Tutorial 04: Static File Handler](04_Static_File_Handler.md) →**
