# Tutorial 01: HTTP Protocol Basics

## 📖 What You'll Learn

- What HTTP is and how it works
- The anatomy of HTTP requests and responses
- Status codes and their meanings
- Headers and why they matter
- How clients and servers communicate

---

## 🌐 What is HTTP?

**HTTP** (HyperText Transfer Protocol) is the foundation of data communication on the web.

### Key Concepts:

1. **Client-Server Model**
   ```
   Client (Browser)  ←→  Server (Your Webserver)
        |                       |
    Sends Request          Sends Response
   ```

2. **Request-Response Cycle**
   - Client sends a **request** (e.g., "give me index.html")
   - Server processes it and sends a **response** (e.g., "here's the HTML")
   - Connection can close or stay open (keep-alive)

3. **Stateless Protocol**
   - Each request is independent
   - Server doesn't remember previous requests
   - That's why we need cookies/sessions for login

---

## 📝 Anatomy of an HTTP Request

### Format:
```
REQUEST_LINE
HEADERS
[empty line]
BODY (optional)
```

### Example GET Request:
```http
GET /index.html?user=john HTTP/1.1
Host: www.example.com
User-Agent: Mozilla/5.0
Accept: text/html
Connection: keep-alive

```

### Breaking it Down:

#### 1. **Request Line** (First Line)
```
METHOD  URI  VERSION
  ↓      ↓      ↓
GET /index.html HTTP/1.1
```

- **METHOD**: What action to perform (GET, POST, DELETE)
- **URI**: What resource (can include query string)
- **VERSION**: HTTP version (we use HTTP/1.1)

#### 2. **Headers** (Key: Value pairs)
```
Host: www.example.com          ← Which server
User-Agent: Mozilla/5.0        ← What client
Accept: text/html              ← What content types accepted
Connection: keep-alive         ← Connection preference
```

#### 3. **Empty Line** (`\r\n\r\n`)
- Separates headers from body
- **Critical**: This is how we know headers ended

#### 4. **Body** (Optional)
- GET requests usually have no body
- POST requests have body (form data, JSON, files)

### Example POST Request:
```http
POST /api/login HTTP/1.1
Host: www.example.com
Content-Type: application/x-www-form-urlencoded
Content-Length: 29

username=john&password=secret
```

---

## 📤 Anatomy of an HTTP Response

### Format:
```
STATUS_LINE
HEADERS
[empty line]
BODY
```

### Example Response:
```http
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 138
Server: WebServ/1.0

<html>
<head><title>Success</title></head>
<body><h1>Welcome!</h1></body>
</html>
```

### Breaking it Down:

#### 1. **Status Line**
```
VERSION  STATUS_CODE  REASON_PHRASE
   ↓          ↓            ↓
HTTP/1.1    200          OK
```

#### 2. **Headers**
```
Content-Type: text/html      ← What type of content
Content-Length: 138          ← How many bytes in body
Server: WebServ/1.0          ← Server identification
```

#### 3. **Body**
- The actual content (HTML, JSON, image data, etc.)

---

## 🔢 HTTP Status Codes

### Why They Matter:
Status codes tell the client what happened with their request.

### Categories:

#### **2xx - Success** ✅
- `200 OK` - Request succeeded
- `201 Created` - Resource was created (after POST)
- `204 No Content` - Success but no body to send

#### **3xx - Redirection** ↪️
- `301 Moved Permanently` - Resource moved forever
- `302 Found` - Resource temporarily at different URI
- `304 Not Modified` - Use cached version

#### **4xx - Client Error** ❌
- `400 Bad Request` - Malformed request
- `403 Forbidden` - Server refuses (permissions)
- `404 Not Found` - Resource doesn't exist
- `405 Method Not Allowed` - Wrong HTTP method
- `413 Payload Too Large` - Request body too big

#### **5xx - Server Error** 💥
- `500 Internal Server Error` - Server crashed/bug
- `501 Not Implemented` - Feature not supported
- `503 Service Unavailable` - Server overloaded

### In Our Implementation:
```cpp
// From HttpResponse.cpp
std::string HttpResponse::getStatusMessage(int code) const {
    switch (code) {
        case 200: return "OK";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        // ... etc
    }
}
```

---

## 📋 Common HTTP Headers

### Request Headers:

| Header | Purpose | Example |
|--------|---------|---------|
| `Host` | Which server/domain | `Host: example.com` |
| `User-Agent` | Client identification | `User-Agent: curl/7.64.1` |
| `Accept` | What content types OK | `Accept: text/html` |
| `Content-Type` | Type of body data | `Content-Type: application/json` |
| `Content-Length` | Size of body in bytes | `Content-Length: 1234` |
| `Connection` | Connection preference | `Connection: keep-alive` |

### Response Headers:

| Header | Purpose | Example |
|--------|---------|---------|
| `Content-Type` | Type of content | `Content-Type: text/html` |
| `Content-Length` | Size of body | `Content-Length: 5678` |
| `Server` | Server software | `Server: WebServ/1.0` |
| `Location` | Redirect target | `Location: /new-page` |

### Why Content-Length Matters:
```
Without it: Client doesn't know when response ends
With it:    Client reads exactly N bytes then stops

Example:
Content-Length: 13
Body: Hello, World!
      ↑           ↑
    byte 0     byte 12
```

---

## 🔄 The Full HTTP Transaction

```
1. Client Connects to Server
   ┌─────────┐         ┌─────────┐
   │ Client  │────────>│ Server  │
   └─────────┘         └─────────┘
   
2. Client Sends Request
   GET /page.html HTTP/1.1
   Host: example.com
   [headers...]
   
3. Server Parses Request
   - Extract method (GET)
   - Extract URI (/page.html)
   - Extract headers
   
4. Server Processes Request
   - Find the file
   - Read content
   - Determine MIME type
   
5. Server Sends Response
   HTTP/1.1 200 OK
   Content-Type: text/html
   Content-Length: 1234
   
   <html>...</html>
   
6. Connection Closes (or stays open)
```

---

## 🎯 Why We Need Each Component

### 1. **Request Parser** (Tutorial 02)
- Converts raw bytes into structured data
- Validates the request
- Extracts method, URI, headers, body

### 2. **Response Generator** (Tutorial 03)
- Builds properly formatted HTTP responses
- Sets correct status codes
- Manages headers

### 3. **Static File Handler** (Tutorial 04)
- Reads files from disk
- Determines MIME type
- Serves content

### 4. **Upload Handler** (Tutorial 05)
- Parses multipart/form-data
- Extracts uploaded files
- Saves to disk

---

## 💡 Key Takeaways

1. **HTTP is text-based** - You can read/write it by hand
2. **Line endings matter** - Must be `\r\n` (CRLF)
3. **Empty line is critical** - Separates headers from body
4. **Headers are case-insensitive** - `Content-Type` = `content-type`
5. **Status codes communicate results** - Client knows what happened
6. **Content-Length is required** - For requests/responses with body

---

## 🔍 Common Questions for Defense

**Q: Why HTTP/1.1 and not HTTP/2?**
- HTTP/1.1 is text-based, easier to implement
- HTTP/2 is binary, requires more complex parsing
- HTTP/1.1 is still widely used and sufficient

**Q: What's the difference between GET and POST?**
- GET: Retrieve data, no body, idempotent (safe to retry)
- POST: Submit data, has body, not idempotent

**Q: Why do we need Content-Length?**
- Tells the receiver how many bytes to read
- Without it, we don't know when the body ends
- Alternative: chunked transfer encoding (more complex)

**Q: What happens if headers are too large?**
- We return `431 Request Header Fields Too Large`
- Prevents DoS attacks with huge headers
- See `HttpRequest.cpp` line ~106

**Q: Why separate request line from headers?**
- Different parsing logic
- Request line has fixed format (3 parts)
- Headers are key-value pairs (variable)

---

## 📚 Next Steps

- **Next**: [Tutorial 02 - HTTP Request Parser](02_HTTP_Request_Parser.md)
- **Code**: See `includes/HttpRequest.hpp` and `srcs/HttpRequest.cpp`
- **Test**: Try `telnet localhost 8080` and type HTTP manually

---

## 🧪 Try It Yourself

### Exercise 1: Send HTTP Request with Telnet
```bash
telnet localhost 8080
GET / HTTP/1.1
Host: localhost
[Press Enter twice]
```

### Exercise 2: Use curl with Verbose
```bash
curl -v http://localhost:8080/index.html
# Watch the request and response
```

### Exercise 3: Inspect in Browser
1. Open browser DevTools (F12)
2. Go to Network tab
3. Visit any page
4. Click on a request
5. See Headers tab

---

**Continue to [Tutorial 02: HTTP Request Parser](02_HTTP_Request_Parser.md) →**
