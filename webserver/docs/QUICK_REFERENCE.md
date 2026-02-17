# Quick Reference Card - HTTP Components

## HttpRequest - Request Parser

### Basic Usage
```cpp
HttpRequest request;
bool complete = request.parse(buffer, bytes_read);

if (request.isComplete()) {
    // Request fully received
    std::string method = request.getMethodString();  // "GET", "POST", "DELETE"
    std::string uri = request.getUri();              // "/path/to/resource"
    std::string query = request.getQueryString();    // "param1=value1&param2=value2"
    std::string body = request.getBody();            // Request body content
}
```

### Checking Method
```cpp
switch (request.getMethod()) {
    case GET:
        // Handle GET
        break;
    case POST:
        // Handle POST
        break;
    case DELETE:
        // Handle DELETE
        break;
    default:
        // Unknown method
        break;
}
```

### Getting Headers
```cpp
std::string host = request.getHeader("Host");
std::string content_type = request.getHeader("Content-Type");
size_t content_len = request.getContentLength();
```

### Error Handling
```cpp
if (!request.isValid()) {
    int error = request.getErrorCode();  // 400, 405, 431, 501, 505, etc.
    // Send error response
}
```

### Multipart Upload Info
```cpp
std::string boundary = request.getBoundary();  // For multipart/form-data
```

---

## HttpResponse - Response Generator

### Quick Responses
```cpp
// 200 OK
HttpResponse resp = HttpResponse::ok("<h1>Success</h1>", "text/html");

// 404 Not Found
HttpResponse resp = HttpResponse::notFound();

// 400 Bad Request
HttpResponse resp = HttpResponse::badRequest("Invalid parameters");

// 405 Method Not Allowed
HttpResponse resp = HttpResponse::methodNotAllowed();

// 500 Internal Server Error
HttpResponse resp = HttpResponse::internalServerError();

// 302 Redirect
HttpResponse resp = HttpResponse::redirect("/new-location");

// 201 Created
HttpResponse resp = HttpResponse::created("/resource/123");

// 204 No Content
HttpResponse resp = HttpResponse::noContent();
```

### Custom Response
```cpp
HttpResponse resp(200);
resp.setContentType("application/json");
resp.setHeader("X-Custom-Header", "value");
resp.setBody("{\"status\": \"ok\"}");

std::string output = resp.build();  // Get complete HTTP response string
```

---

## StaticFileHandler - Static File Serving

### Setup
```cpp
StaticFileHandler handler(
    "./www",           // Root directory
    true,              // Enable directory listing
    "index.html"       // Default file
);
```

### Handle Request
```cpp
HttpResponse response = handler.handleRequest(request);
std::string output = response.build();
// Send output to client
```

### Configuration
```cpp
handler.setRootDirectory("./public");
handler.setDirectoryListing(false);  // Disable directory listing
handler.setDefaultFile("home.html");
```

### Features
- Automatic MIME type detection
- Directory listing with HTML formatting
- Default file serving
- Path traversal protection
- 404 handling

---

## UploadHandler - File Upload Handler

### Setup
```cpp
UploadHandler handler(
    "./uploads",      // Upload directory
    10485760          // Max size: 10MB
);
```

### Handle Upload
```cpp
if (request.getMethod() == POST && request.getUri() == "/upload") {
    HttpResponse response = handler.handleUpload(request);
    // Response contains upload status
}
```

### Configuration
```cpp
handler.setUploadDirectory("./user-uploads");
handler.setMaxUploadSize(5242880);  // 5MB
```

### Features
- Multipart/form-data parsing
- Multiple file support
- Filename sanitization
- Size validation
- Auto-creates upload directory

---

## Common Patterns

### Basic Request Handling
```cpp
HttpRequest req;
if (req.parse(data, len) && req.isComplete()) {
    HttpResponse resp;
    
    if (!req.isValid()) {
        resp.setStatusCode(req.getErrorCode());
    } else if (req.getMethod() == GET) {
        StaticFileHandler handler("./www");
        resp = handler.handleRequest(req);
    } else if (req.getMethod() == POST) {
        UploadHandler uploader("./uploads");
        resp = uploader.handleUpload(req);
    } else if (req.getMethod() == DELETE) {
        resp = HttpResponse::noContent();
    } else {
        resp = HttpResponse::methodNotAllowed();
    }
    
    std::string output = resp.build();
    send(socket, output.c_str(), output.length(), 0);
}
```

### Incremental Parsing (for poll-based server)
```cpp
// In your read handler
char buffer[4096];
ssize_t n = recv(fd, buffer, sizeof(buffer), 0);

if (n > 0) {
    bool complete = client->request.parse(buffer, n);
    
    if (complete) {
        // Process request and generate response
        HttpResponse resp = processRequest(client->request);
        client->response_buffer = resp.build();
        client->ready_to_write = true;
    }
}
```

### MIME Type Examples
```
.html, .htm  -> text/html
.css         -> text/css
.js          -> application/javascript
.json        -> application/json
.jpg, .jpeg  -> image/jpeg
.png         -> image/png
.gif         -> image/gif
.svg         -> image/svg+xml
.pdf         -> application/pdf
.txt         -> text/plain
.mp4         -> video/mp4
.mp3         -> audio/mpeg
```

### HTTP Status Codes
```
200 - OK
201 - Created
204 - No Content
301 - Moved Permanently
302 - Found (Redirect)
400 - Bad Request
403 - Forbidden
404 - Not Found
405 - Method Not Allowed
413 - Payload Too Large
431 - Request Header Fields Too Large
500 - Internal Server Error
501 - Not Implemented
505 - HTTP Version Not Supported
```

---

## Testing Commands

### Telnet
```bash
telnet localhost 8080
GET /index.html HTTP/1.1
Host: localhost
[Press Enter twice]
```

### curl
```bash
# GET request
curl -v http://localhost:8080/test.html

# POST with form data
curl -X POST -d "name=test" http://localhost:8080/api/submit

# File upload
curl -F "file=@myfile.txt" http://localhost:8080/upload

# DELETE request
curl -X DELETE http://localhost:8080/api/resource/123
```

### Python
```bash
python3 test_parser.py   # Test request parser
python3 test_upload.py   # Test file uploads
```

---

## Notes

- All components are **C++98 compliant**
- **Non-blocking safe** - can be used with poll()
- **Cross-platform** - Works on Windows and Unix
- **No external dependencies** - Only standard library
- **Thread-safe** - Each request/response is independent
- **Security** - Path traversal protection, filename sanitization
