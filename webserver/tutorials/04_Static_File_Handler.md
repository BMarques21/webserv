# Tutorial 04: Static File Handler

## 📖 What You'll Learn

- How to serve files from disk
- MIME type detection
- Directory listing generation
- Security (path traversal attacks)
- Cross-platform file operations
- Performance considerations

---

## 🎯 The Goal

Transform this:
```http
GET /images/logo.png HTTP/1.1
```

Into this:
```http
HTTP/1.1 200 OK
Content-Type: image/png
Content-Length: 15234

[binary PNG data]
```

---

## 🏗️ Architecture Overview

### Class Design

```cpp
class StaticFileHandler {
private:
    std::string base_path;  // Root directory (e.g., "/var/www/")
    
    std::string getMimeType(const std::string& filename);
    std::string readFile(const std::string& path);
    std::string generateDirectoryListing(const std::string& dir_path,
                                         const std::string& uri);
    bool isDirectory(const std::string& path);
    std::string sanitizePath(const std::string& path);
    
public:
    StaticFileHandler(const std::string& base);
    HttpResponse handle(const HttpRequest& request);
};
```

### Workflow

```
Request URI
    ↓
Sanitize (security check)
    ↓
Combine with base_path
    ↓
Check if exists
    ↓
Is directory? → Generate listing
    ↓
Is file? → Read file → Detect MIME → Send
    ↓
Not found? → 404 error
```

---

## 🗂️ MIME Type Detection

### What is MIME?

**MIME** = Multipurpose Internet Mail Extensions
- Tells browser what type of content to expect
- Format: `type/subtype`

Examples:
```
text/html       → HTML documents
text/css        → Stylesheets
application/json → JSON data
image/png       → PNG images
video/mp4       → MP4 videos
```

### The Mapping Table

```cpp
std::string StaticFileHandler::getMimeType(const std::string& filename) {
    // Find last dot
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "application/octet-stream";  // Binary default
    }
    
    std::string extension = filename.substr(dot_pos);
    
    // Extension to MIME mapping
    if (extension == ".html" || extension == ".htm") 
        return "text/html";
    if (extension == ".css") 
        return "text/css";
    if (extension == ".js") 
        return "application/javascript";
    if (extension == ".json") 
        return "application/json";
    if (extension == ".png") 
        return "image/png";
    if (extension == ".jpg" || extension == ".jpeg") 
        return "image/jpeg";
    if (extension == ".gif") 
        return "image/gif";
    if (extension == ".svg") 
        return "image/svg+xml";
    if (extension == ".ico") 
        return "image/x-icon";
    if (extension == ".txt") 
        return "text/plain";
    if (extension == ".pdf") 
        return "application/pdf";
    if (extension == ".zip") 
        return "application/zip";
    if (extension == ".mp4") 
        return "video/mp4";
    if (extension == ".mp3") 
        return "audio/mpeg";
    
    // Default for unknown types
    return "application/octet-stream";
}
```

### Why Extension-Based?

**Alternatives:**
1. **Magic numbers** - Read file content, check signature
   - PNG: `89 50 4E 47` (first 4 bytes)
   - JPEG: `FF D8 FF`
   - More accurate but slower

2. **Extension** (our choice)
   - Fast (no file I/O)
   - Good enough for web server
   - Standard practice

### What is `application/octet-stream`?

- Generic binary type
- Browser will download it (not display)
- Used when type is unknown

---

## 📂 Reading Files

### Implementation

```cpp
std::string StaticFileHandler::readFile(const std::string& path) {
    std::ifstream file(path.c_str(), std::ios::binary);
    
    if (!file.is_open()) {
        return "";  // File not found or can't open
    }
    
    // Get file size
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read into string
    std::string content;
    content.resize(size);
    file.read(&content[0], size);
    
    file.close();
    return content;
}
```

### Why Binary Mode?

```cpp
std::ifstream file(path.c_str(), std::ios::binary);
//                                 ^^^^^^^^^^^^^^^ Important!
```

**Without binary:**
- Text mode may convert line endings
- `\n` → `\r\n` on Windows
- Corrupts binary files (images, PDFs, etc.)

**With binary:**
- Raw bytes, no conversion
- Works for both text and binary files

### Why resize()?

```cpp
std::string content;
content.resize(size);         // Pre-allocate memory
file.read(&content[0], size);  // Read directly into string
```

**Benefits:**
- One allocation (fast)
- No reallocations during read
- Direct buffer access

**Alternative (slower):**
```cpp
std::string content;
char buffer[1024];
while (file.read(buffer, 1024)) {
    content.append(buffer, file.gcount());  // Multiple allocations
}
```

---

## 📊 Directory Listing

### When to Show?

```
Request: GET /images/ HTTP/1.1
                     ^ Trailing slash = directory
```

**Options:**
1. Show directory listing (our choice)
2. Serve index.html if exists
3. Return 403 Forbidden

### Implementation (Linux)

```cpp
std::string StaticFileHandler::generateDirectoryListing(
    const std::string& dir_path,
    const std::string& uri) {
    
    std::ostringstream html;
    
    // HTML header
    html << "<html><head><title>Index of " << uri << "</title></head>"
         << "<body><h1>Index of " << uri << "</h1>"
         << "<hr><ul>";
    
    // Open directory
    DIR* dir = opendir(dir_path.c_str());
    if (!dir) {
        return "";  // Can't open directory
    }
    
    // Read entries
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        
        // Skip hidden files
        if (name[0] == '.') continue;
        
        // Build URI for link
        std::string link_uri = uri;
        if (link_uri[link_uri.length() - 1] != '/') {
            link_uri += '/';
        }
        link_uri += name;
        
        // Add entry type indicator
        std::string full_path = dir_path + "/" + name;
        if (isDirectory(full_path)) {
            name += '/';  // Append / for directories
        }
        
        // HTML list item
        html << "<li><a href=\"" << link_uri << "\">" 
             << name << "</a></li>";
    }
    
    closedir(dir);
    
    html << "</ul><hr></body></html>";
    return html.str();
}
```

### Example Output

**Request:** `GET /uploads/`

**Response:**
```html
<html>
<head><title>Index of /uploads/</title></head>
<body>
  <h1>Index of /uploads/</h1>
  <hr>
  <ul>
    <li><a href="/uploads/document.pdf">document.pdf</a></li>
    <li><a href="/uploads/image.png">image.png</a></li>
    <li><a href="/uploads/videos/">videos/</a></li>
  </ul>
  <hr>
</body>
</html>
```

### Linux vs Windows

**Linux (POSIX):**
```cpp
#include <dirent.h>

DIR* dir = opendir(path);
struct dirent* entry;
while ((entry = readdir(dir)) != NULL) {
    std::string name = entry->d_name;
}
closedir(dir);
```

**Windows (Win32 API):**
```cpp
#include <windows.h>

WIN32_FIND_DATA data;
HANDLE hFind = FindFirstFile((path + "\\*").c_str(), &data);
do {
    std::string name = data.cFileName;
} while (FindNextFile(hFind, &data));
FindClose(hFind);
```

**Our Choice:** Primary Linux (POSIX), Windows stub only

---

## 🔒 Security - Path Traversal

### The Attack

```http
GET /../../../etc/passwd HTTP/1.1
     ^^^^^^^^^^^^^^^^^ Trying to escape base_path!
```

**Goal:** Access files outside allowed directory

### Vulnerable Code

```cpp
// BAD - Don't do this!
std::string full_path = base_path + request.getUri();
// If uri = "/../../../etc/passwd"
// full_path = "/var/www/../../../etc/passwd"
//           = "/etc/passwd"  ← DANGER!
```

### The Fix: Path Sanitization

```cpp
std::string StaticFileHandler::sanitizePath(const std::string& path) {
    std::string result;
    
    // Split by '/'
    std::vector<std::string> parts;
    size_t start = 0;
    size_t pos = path.find('/');
    
    while (pos != std::string::npos) {
        std::string part = path.substr(start, pos - start);
        if (!part.empty() && part != ".") {
            parts.push_back(part);
        }
        start = pos + 1;
        pos = path.find('/', start);
    }
    
    // Add last part
    if (start < path.length()) {
        std::string part = path.substr(start);
        if (!part.empty() && part != ".") {
            parts.push_back(part);
        }
    }
    
    // Process ".." (parent directory)
    std::vector<std::string> clean_parts;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (parts[i] == "..") {
            // Go up one level (remove last part)
            if (!clean_parts.empty()) {
                clean_parts.pop_back();
            }
        } else {
            clean_parts.push_back(parts[i]);
        }
    }
    
    // Rebuild path
    result = "/";
    for (size_t i = 0; i < clean_parts.size(); ++i) {
        result += clean_parts[i];
        if (i < clean_parts.size() - 1) {
            result += "/";
        }
    }
    
    return result;
}
```

### Example Sanitization

| Input | Output |
|-------|--------|
| `/images/../etc/passwd` | `/etc/passwd` |
| `/./index.html` | `/index.html` |
| `/images//logo.png` | `/images/logo.png` |
| `/images/../../secret` | `/secret` |

### Final Check

```cpp
HttpResponse StaticFileHandler::handle(const HttpRequest& request) {
    // 1. Sanitize URI
    std::string uri = sanitizePath(request.getUri());
    
    // 2. Build full path
    std::string full_path = base_path + uri;
    
    // 3. Security check: ensure path starts with base_path
    if (full_path.find(base_path) != 0) {
        return HttpResponse::forbidden("Access denied");
    }
    
    // ... continue with file serving
}
```

**Why the check?**
```
base_path = "/var/www"
uri = "/../../../etc/passwd"
full_path = "/var/www/../../../etc/passwd" = "/etc/passwd"

full_path.find(base_path) = not found!
→ Access denied ✓
```

---

## 🔄 Complete Request Flow

### Putting It All Together

```cpp
HttpResponse StaticFileHandler::handle(const HttpRequest& request) {
    // 1. Security: Sanitize URI
    std::string uri = sanitizePath(request.getUri());
    
    // 2. Build full filesystem path
    std::string full_path = base_path + uri;
    
    // 3. Verify path is within base_path (security)
    if (full_path.find(base_path) != 0) {
        return HttpResponse::forbidden("Access denied");
    }
    
    // 4. Check if path exists
    struct stat path_stat;
    if (stat(full_path.c_str(), &path_stat) != 0) {
        return HttpResponse::notFound("File not found: " + uri);
    }
    
    // 5. Is it a directory?
    if (S_ISDIR(path_stat.st_mode)) {
        std::string listing = generateDirectoryListing(full_path, uri);
        if (listing.empty()) {
            return HttpResponse::forbidden("Cannot list directory");
        }
        return HttpResponse::ok(listing, "text/html");
    }
    
    // 6. Is it a file?
    if (S_ISREG(path_stat.st_mode)) {
        std::string content = readFile(full_path);
        if (content.empty()) {
            return HttpResponse::internalServerError("Cannot read file");
        }
        
        std::string mime_type = getMimeType(full_path);
        return HttpResponse::ok(content, mime_type);
    }
    
    // 7. Neither file nor directory (symlink, device, etc.)
    return HttpResponse::forbidden("Invalid resource type");
}
```

### Flow Diagram

```
GET /images/logo.png
        ↓
Sanitize path → /images/logo.png
        ↓
Full path = /var/www/images/logo.png
        ↓
Security check: starts with /var/www? → Yes ✓
        ↓
Exists? → Yes ✓
        ↓
Directory? → No
        ↓
File? → Yes ✓
        ↓
Read file → [binary PNG data]
        ↓
MIME type = image/png
        ↓
HTTP/1.1 200 OK
Content-Type: image/png
Content-Length: 15234
```

---

## ⚡ Performance Considerations

### File Size Limits

```cpp
std::string StaticFileHandler::readFile(const std::string& path) {
    std::ifstream file(path.c_str(), std::ios::binary);
    
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    
    // Limit: 10MB
    if (size > 10 * 1024 * 1024) {
        return "";  // File too large
    }
    
    // ... continue reading
}
```

**Why limit?**
- Reading 1GB file → 1GB RAM usage
- Multiple requests → memory exhaustion
- Better to use chunked transfer (advanced)

### Caching Headers

```cpp
HttpResponse response = HttpResponse::ok(content, mime_type);

// Add cache headers
response.setHeader("Cache-Control", "public, max-age=3600");  // 1 hour
response.setHeader("Last-Modified", getLastModifiedTime(full_path));

return response;
```

**Benefits:**
- Browser caches file
- Subsequent requests faster
- Reduces server load

### stat() vs. Multiple Calls

```cpp
// Efficient (one system call):
struct stat path_stat;
stat(full_path.c_str(), &path_stat);
bool is_dir = S_ISDIR(path_stat.st_mode);
bool is_file = S_ISREG(path_stat.st_mode);
size_t size = path_stat.st_size;

// Inefficient (multiple system calls):
bool is_dir = isDirectory(full_path);   // stat() call 1
bool is_file = isFile(full_path);        // stat() call 2
size_t size = getFileSize(full_path);    // stat() call 3
```

---

## 💡 Design Decisions

### Why Not Use fstream for Existence Check?

```cpp
// Don't do this:
std::ifstream file(path);
if (file.good()) {
    // File exists
}
```

**Problems:**
- Opens file (wasteful)
- Can't distinguish file vs. directory
- Doesn't give file metadata

**Better:**
```cpp
struct stat path_stat;
if (stat(path.c_str(), &path_stat) == 0) {
    // Exists, and we have metadata!
}
```

### Why Return Empty String on Error?

```cpp
std::string readFile(const std::string& path) {
    if (!file.is_open()) {
        return "";  // Empty = error
    }
    // ...
}
```

**Alternatives:**
1. **Throw exception** - C++98 doesn't have great exception support
2. **Return pair<bool, string>** - Not available (C++11)
3. **Output parameter** - Messy

**Our choice:** Empty string = error
- Simple
- Caller checks with `.empty()`
- Caller can differentiate (0-byte file has stat() success)

### Why Separate MIME Detection?

```cpp
// Separate method
std::string mime = getMimeType(filename);

// vs. Inline
if (filename.ends_with(".html")) { ... }
```

**Benefits:**
- Reusable
- Testable in isolation
- Easy to extend (add new types)
- Centralized mapping

---

## 🔍 Real-World Scenarios

### Scenario 1: Website with Assets

**Directory structure:**
```
/var/www/
  ├── index.html
  ├── styles/
  │   └── main.css
  └── images/
      └── logo.png
```

**Requests:**
```http
GET / HTTP/1.1
→ Serve /var/www/index.html (text/html)

GET /styles/main.css HTTP/1.1
→ Serve /var/www/styles/main.css (text/css)

GET /images/logo.png HTTP/1.1
→ Serve /var/www/images/logo.png (image/png)
```

### Scenario 2: Download Server

```http
GET /downloads/ HTTP/1.1
→ Directory listing of available files

GET /downloads/software-v1.2.zip HTTP/1.1
→ Serve file with Content-Type: application/zip
→ Browser downloads (not displays)
```

### Scenario 3: Attack Attempt

```http
GET /../../../etc/passwd HTTP/1.1
→ sanitizePath() normalizes to /etc/passwd
→ full_path = /var/www/etc/passwd
→ Security check: doesn't start with /var/www/etc/
→ 403 Forbidden ✓
```

---

## 🔍 Common Questions for Defense

**Q: Why not use nginx or Apache?**
- This is a learning project
- Understanding low-level file serving
- Customization for specific needs

**Q: What if two files have the same name but different cases?**
```
/images/Logo.png
/images/logo.png
```
- Linux: Different files (case-sensitive)
- Windows: Same file (case-insensitive)
- Our code works on both (uses OS filesystem rules)

**Q: How do you handle large files (GB+)?**
- Current: Load entire file into memory (limited to 10MB)
- Better: Chunked transfer encoding (send pieces)
- Best: sendfile() system call (zero-copy)

**Q: What about symbolic links?**
- stat() follows symlinks by default
- Could use lstat() to detect them
- Security risk if symlink points outside base_path
- Consider checking and rejecting symlinks

**Q: Why generate HTML for directory listings?**
- Better than plain text (clickable links)
- Readable in browser
- Can add CSS styling
- Standard practice (Apache, nginx do this)

---

## 📚 Code References

### Files:
- `includes/StaticFileHandler.hpp` - Class definition
- `srcs/StaticFileHandler.cpp` - Implementation

### Key Methods:
```cpp
HttpResponse handle(const HttpRequest& req);           // Main handler
std::string getMimeType(const std::string& filename);  // MIME detection
std::string readFile(const std::string& path);         // File I/O
std::string generateDirectoryListing(...);             // List directory
std::string sanitizePath(const std::string& path);     // Security
```

### System Calls Used:
```cpp
stat()      // File metadata
opendir()   // Open directory
readdir()   // Read directory entries
closedir()  // Close directory
```

---

## 🧪 Testing Your Understanding

### Exercise 1: MIME Types
What Content-Type should be sent for:
1. `report.pdf`
2. `data.json`
3. `video.mp4`
4. `unknown.xyz`

### Exercise 2: Path Traversal
Are these safe? Why or why not?
1. `GET /images/../../etc/passwd`
2. `GET /./index.html`
3. `GET //images///logo.png`
4. `GET /uploads/../../www/index.html` (base_path = `/var/www`)

### Exercise 3: Error Handling
What response code for:
1. File doesn't exist?
2. File exists but can't read (permissions)?
3. Path tries to escape base_path?
4. Requested a device file (e.g., `/dev/null`)?

---

**Continue to [Tutorial 05: File Upload Handler](05_File_Upload_Handler.md) →**
