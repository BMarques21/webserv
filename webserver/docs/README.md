# WebServer HTTP Parser and Response Components

This directory contains the HTTP request parser, response generator, static file handler, and upload handler components for the webserver project.

## Components

### 1. HttpRequest (HttpRequest.hpp/cpp)
HTTP request parser that supports:
- **GET, POST, DELETE** methods
- Request line parsing (method, URI, HTTP version)
- Header parsing (case-insensitive)
- Query string extraction
- Body parsing based on Content-Length
- Multipart/form-data boundary extraction
- Request validation with proper error codes

**Features:**
- Incremental parsing (can handle data in chunks)
- Returns appropriate HTTP error codes (400, 405, 431, 505, etc.)
- Query string parsing
- Support for Content-Length based body reading

### 2. HttpResponse (HttpResponse.hpp/cpp)
HTTP response generator that provides:
- Status line generation
- Header management
- Body content with automatic Content-Length
- Helper methods for common responses (200, 201, 204, 302, 400, 404, 405, 413, 500, 501)

**Static helper methods:**
- `ok()` - 200 OK
- `created()` - 201 Created
- `noContent()` - 204 No Content
- `redirect()` - 302/301 Redirect
- `badRequest()` - 400 Bad Request
- `notFound()` - 404 Not Found
- `methodNotAllowed()` - 405 Method Not Allowed
- `payloadTooLarge()` - 413 Payload Too Large
- `internalServerError()` - 500 Internal Server Error
- `notImplemented()` - 501 Not Implemented

### 3. StaticFileHandler (StaticFileHandler.hpp/cpp)
Serves static files with:
- **MIME type detection** (HTML, CSS, JS, images, fonts, etc.)
- **Directory listing** (can be enabled/disabled)
- Default file serving (index.html)
- Path traversal protection (prevents ../ attacks)
- Cross-platform file operations (Windows and Unix)

**Supported MIME types:**
- text/html, text/css, text/plain
- application/javascript, application/json
- image/jpeg, image/png, image/gif, image/svg+xml
- font/woff, font/woff2, font/ttf
- video/mp4, audio/mpeg
- application/pdf, application/zip
- And more...

### 4. UploadHandler (UploadHandler.hpp/cpp)
Handles file uploads with:
- **multipart/form-data** parsing
- Multiple file upload support
- File size validation
- Filename sanitization (security)
- Configurable upload directory
- Configurable max upload size

## Building

### Using Make (Linux/Unix/WSL):
```bash
make
```

### Manual compilation:
```bash
g++ -Wall -Wextra -Werror -std=c++98 -o test_http \
    HttpRequest.cpp HttpResponse.cpp StaticFileHandler.cpp \
    UploadHandler.cpp test_http.cpp
```

### On Windows with g++:
```powershell
g++ -Wall -Wextra -std=c++98 -o test_http.exe HttpRequest.cpp HttpResponse.cpp StaticFileHandler.cpp UploadHandler.cpp test_http.cpp
```

## Testing

### 1. Unit Tests
Run the test program:
```bash
./test_http
```

### 2. Python Test Scripts

**Parser tests with raw sockets:**
```bash
python3 test_parser.py
```

**Upload tests:**
```bash
python3 test_upload.py
```

### 3. Manual Testing with Telnet

**Test GET request:**
```bash
telnet localhost 8080
GET /test.html HTTP/1.1
Host: localhost
[Press Enter twice]
```

**Test POST request:**
```bash
telnet localhost 8080
POST /api/test HTTP/1.1
Host: localhost
Content-Length: 13
[Press Enter]
name=testuser
```

### 4. Testing with curl

**GET request:**
```bash
curl -v http://localhost:8080/test.html
```

**POST with form data:**
```bash
curl -X POST -d "name=test&value=123" http://localhost:8080/api/submit
```

**File upload:**
```bash
curl -F "file=@testfile.txt" http://localhost:8080/upload
```

**DELETE request:**
```bash
curl -X DELETE http://localhost:8080/api/items/123
```

## Usage Example

```cpp
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "StaticFileHandler.hpp"
#include "UploadHandler.hpp"

// Parse a request
HttpRequest request;
bool complete = request.parse(buffer, bytes_received);

if (request.isComplete()) {
    HttpResponse response;
    
    if (request.getUri() == "/upload" && request.getMethod() == POST) {
        // Handle file upload
        UploadHandler uploader("./uploads", 10485760);
        response = uploader.handleUpload(request);
    } else if (request.getMethod() == GET) {
        // Serve static file
        StaticFileHandler handler("./www", true);
        response = handler.handleRequest(request);
    } else if (request.getMethod() == DELETE) {
        // Handle DELETE
        response = HttpResponse::noContent();
    } else {
        response = HttpResponse::methodNotAllowed();
    }
    
    // Send response
    std::string response_str = response.build();
    send(socket, response_str.c_str(), response_str.length(), 0);
}
```

## Directory Structure

```
webserver/
├── HttpRequest.hpp          # Request parser header
├── HttpRequest.cpp          # Request parser implementation
├── HttpResponse.hpp         # Response generator header
├── HttpResponse.cpp         # Response generator implementation
├── StaticFileHandler.hpp    # Static file handler header
├── StaticFileHandler.cpp    # Static file handler implementation
├── UploadHandler.hpp        # Upload handler header
├── UploadHandler.cpp        # Upload handler implementation
├── test_http.cpp           # Unit test program
├── test_parser.py          # Python parser tests (telnet-style)
├── test_upload.py          # Python upload tests
├── Makefile               # Build configuration
├── README.md              # This file
├── www/                   # Static files directory
│   ├── index.html
│   ├── test.html
│   ├── style.css
│   └── script.js
└── uploads/               # Upload directory (create this)
```

## Key Features

### Request Parser
- ✅ Supports GET, POST, DELETE methods
- ✅ Parses request line, headers, and body
- ✅ Query string extraction
- ✅ Error validation with proper HTTP codes
- ✅ Incremental parsing (chunk-by-chunk)
- ✅ Content-Length based body reading
- ✅ Multipart boundary extraction

### Response Generator
- ✅ Proper HTTP/1.1 status lines
- ✅ Header management
- ✅ Automatic Content-Length calculation
- ✅ Helper methods for common responses
- ✅ Custom error pages

### Static File Handler
- ✅ MIME type detection (25+ types)
- ✅ Directory listing with HTML formatting
- ✅ Default file serving (index.html)
- ✅ Path traversal protection
- ✅ Cross-platform support

### Upload Handler
- ✅ Multipart/form-data parsing
- ✅ Multiple file support
- ✅ File size validation
- ✅ Filename sanitization
- ✅ Configurable upload limits

## Notes

- All code is C++98 compliant
- Works on both Windows and Unix systems
- Uses only standard library functions
- No external dependencies required
- Ready to integrate with poll()-based server
- Thread-safe (can be used in multi-connection scenarios)

## Integration with Main Server

These components are designed to be used with your partner's poll()-based server implementation:

1. **After poll() indicates socket is readable:**
   - Read data from socket
   - Pass to `HttpRequest::parse()`
   - Check `isComplete()` before processing

2. **After processing request:**
   - Create appropriate `HttpResponse`
   - Call `response.build()` to get string
   - Use poll() to wait for socket writability
   - Write response to socket

3. **For static files:**
   - Use `StaticFileHandler::handleRequest()`

4. **For uploads:**
   - Use `UploadHandler::handleUpload()`

## TODO for Full Server Integration

- [ ] Integrate with poll()-based event loop
- [ ] Add configuration file parsing
- [ ] Implement CGI execution
- [ ] Add DELETE file handling (currently only returns response)
- [ ] Implement chunked transfer encoding
- [ ] Add request timeout handling
- [ ] Implement HTTP redirection from config
- [ ] Add virtual host support (bonus)
