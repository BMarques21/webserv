# Tutorial 05: File Upload Handler

## 📖 What You'll Learn

- Multipart/form-data format
- Boundary parsing algorithm
- File extraction from POST body
- Filename sanitization (security)
- File size validation
- Multiple file uploads

---

## 🎯 The Goal

Transform this:
```http
POST /upload HTTP/1.1
Content-Type: multipart/form-data; boundary=----WebKitFormBoundary
Content-Length: 1234

------WebKitFormBoundary
Content-Disposition: form-data; name="file"; filename="photo.jpg"
Content-Type: image/jpeg

[binary image data]
------WebKitFormBoundary--
```

Into:
- Save file as `uploads/photo.jpg`
- Return `201 Created` response

---

## 🏗️ Architecture Overview

### Class Design

```cpp
class UploadHandler {
private:
    std::string upload_dir;           // Where to save files
    size_t max_file_size;             // Size limit (bytes)
    
    std::string extractBoundary(const std::string& content_type);
    std::vector<UploadedFile> parseMultipartFormData(
        const std::string& body,
        const std::string& boundary
    );
    std::string sanitizeFilename(const std::string& filename);
    bool saveFile(const std::string& path, const std::string& content);
    
public:
    UploadHandler(const std::string& dir, size_t max_size);
    HttpResponse handle(const HttpRequest& request);
};

struct UploadedFile {
    std::string field_name;    // Form field name
    std::string filename;      // Original filename
    std::string content_type;  // MIME type
    std::string content;       // File data
};
```

### Workflow

```
POST request
    ↓
Extract boundary from Content-Type header
    ↓
Parse body → Find parts between boundaries
    ↓
For each part:
  - Parse headers (Content-Disposition, Content-Type)
  - Extract filename
  - Extract binary content
    ↓
Sanitize filename (security)
    ↓
Validate size
    ↓
Save to disk
    ↓
Return 201 Created or error
```

---

## 📦 Multipart/Form-Data Format

### What Is It?

HTML form:
```html
<form action="/upload" method="POST" enctype="multipart/form-data">
  <input type="file" name="photo">
  <input type="submit">
</form>
```

Browser sends:
```http
POST /upload HTTP/1.1
Content-Type: multipart/form-data; boundary=----WebKitFormBoundary123
Content-Length: 2048

------WebKitFormBoundary123
Content-Disposition: form-data; name="photo"; filename="vacation.jpg"
Content-Type: image/jpeg

[binary JPEG data here]
------WebKitFormBoundary123--
```

### Anatomy of Multipart

```
------WebKitFormBoundary123                    ← Boundary (starts with --)
Content-Disposition: form-data; name="photo";  ← Part headers
                    filename="vacation.jpg"
Content-Type: image/jpeg
                                                ← Empty line
[binary content here]                          ← File data
------WebKitFormBoundary123--                  ← End boundary (with -- at end)
```

**Key Points:**
1. **Boundary** - Separates parts (random string)
2. **Part headers** - Describe each part
3. **Empty line** - Separates headers from content
4. **End boundary** - Has `--` at the end

---

## 🔍 Extracting the Boundary

### From Content-Type Header

```cpp
std::string UploadHandler::extractBoundary(const std::string& content_type) {
    // Example: "multipart/form-data; boundary=----WebKitFormBoundary123"
    
    size_t pos = content_type.find("boundary=");
    if (pos == std::string::npos) {
        return "";  // No boundary found
    }
    
    // Skip "boundary="
    pos += 9;  // strlen("boundary=")
    
    // Extract until end or semicolon
    size_t end = content_type.find(';', pos);
    if (end == std::string::npos) {
        end = content_type.length();
    }
    
    std::string boundary = content_type.substr(pos, end - pos);
    
    // Remove quotes if present: boundary="abc" → abc
    if (boundary.length() >= 2 && 
        boundary[0] == '"' && 
        boundary[boundary.length() - 1] == '"') {
        boundary = boundary.substr(1, boundary.length() - 2);
    }
    
    return boundary;
}
```

### Example

| Content-Type | Extracted Boundary |
|--------------|-------------------|
| `multipart/form-data; boundary=abc123` | `abc123` |
| `multipart/form-data; boundary="----WebKit"` | `----WebKit` |
| `application/json` | `` (empty) |

---

## 🧩 Parsing Multipart Body

### Algorithm Overview

1. Split body by boundary
2. For each part:
   - Parse headers
   - Extract content
3. Return array of UploadedFile

### Implementation

```cpp
std::vector<UploadedFile> UploadHandler::parseMultipartFormData(
    const std::string& body,
    const std::string& boundary) {
    
    std::vector<UploadedFile> files;
    
    // Boundary in body has -- prefix
    std::string delimiter = "--" + boundary;
    
    // Find all parts
    size_t pos = 0;
    while (pos < body.length()) {
        // Find start of next part
        size_t part_start = body.find(delimiter, pos);
        if (part_start == std::string::npos) break;
        
        part_start += delimiter.length();
        
        // Skip CRLF after boundary
        if (body.substr(part_start, 2) == "\r\n") {
            part_start += 2;
        }
        
        // Find end of this part (next boundary)
        size_t part_end = body.find(delimiter, part_start);
        if (part_end == std::string::npos) break;
        
        // Extract part content
        std::string part = body.substr(part_start, part_end - part_start);
        
        // Parse this part
        UploadedFile file = parsePart(part);
        if (!file.filename.empty()) {
            files.push_back(file);
        }
        
        pos = part_end;
    }
    
    return files;
}
```

### Parsing a Single Part

```cpp
UploadedFile UploadHandler::parsePart(const std::string& part) {
    UploadedFile file;
    
    // Find empty line that separates headers from content
    size_t header_end = part.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return file;  // Invalid part
    }
    
    // Extract headers section
    std::string headers = part.substr(0, header_end);
    
    // Extract content section (skip the \r\n\r\n)
    std::string content = part.substr(header_end + 4);
    
    // Remove trailing \r\n from content
    if (content.length() >= 2 && content.substr(content.length() - 2) == "\r\n") {
        content = content.substr(0, content.length() - 2);
    }
    
    // Parse headers
    parsePartHeaders(headers, file);
    
    // Store content
    file.content = content;
    
    return file;
}
```

### Parsing Part Headers

```cpp
void UploadHandler::parsePartHeaders(const std::string& headers, 
                                     UploadedFile& file) {
    // Split headers by \r\n
    size_t pos = 0;
    while (pos < headers.length()) {
        size_t line_end = headers.find("\r\n", pos);
        if (line_end == std::string::npos) {
            line_end = headers.length();
        }
        
        std::string line = headers.substr(pos, line_end - pos);
        
        // Content-Disposition: form-data; name="file"; filename="photo.jpg"
        if (line.find("Content-Disposition:") == 0) {
            // Extract name
            size_t name_pos = line.find("name=\"");
            if (name_pos != std::string::npos) {
                name_pos += 6;  // strlen("name=\"")
                size_t name_end = line.find("\"", name_pos);
                file.field_name = line.substr(name_pos, name_end - name_pos);
            }
            
            // Extract filename
            size_t filename_pos = line.find("filename=\"");
            if (filename_pos != std::string::npos) {
                filename_pos += 10;  // strlen("filename=\"")
                size_t filename_end = line.find("\"", filename_pos);
                file.filename = line.substr(filename_pos, 
                                           filename_end - filename_pos);
            }
        }
        
        // Content-Type: image/jpeg
        if (line.find("Content-Type:") == 0) {
            size_t type_pos = line.find(":") + 1;
            while (type_pos < line.length() && line[type_pos] == ' ') {
                type_pos++;
            }
            file.content_type = line.substr(type_pos);
        }
        
        pos = line_end + 2;  // Skip \r\n
    }
}
```

### Visual Example

**Input:**
```
------WebKitFormBoundary123
Content-Disposition: form-data; name="photo"; filename="cat.jpg"
Content-Type: image/jpeg

[JPEG binary data]
------WebKitFormBoundary123--
```

**Parse Steps:**

1. **Find boundary:** `------WebKitFormBoundary123`
2. **Extract part:**
   ```
   Content-Disposition: form-data; name="photo"; filename="cat.jpg"
   Content-Type: image/jpeg
   
   [JPEG binary data]
   ```
3. **Split headers and content:**
   - Headers: `Content-Disposition...\r\nContent-Type...`
   - Content: `[JPEG binary data]`
4. **Parse headers:**
   - `field_name = "photo"`
   - `filename = "cat.jpg"`
   - `content_type = "image/jpeg"`
5. **Result:**
   ```cpp
   UploadedFile {
       field_name: "photo",
       filename: "cat.jpg",
       content_type: "image/jpeg",
       content: [binary data]
   }
   ```

---

## 🔒 Filename Sanitization

### The Threat

```http
POST /upload HTTP/1.1
...
filename="../../../etc/passwd"
```

**Goal:** Overwrite system files or escape upload directory

### Vulnerable Code

```cpp
// BAD - Don't do this!
std::string save_path = upload_dir + "/" + file.filename;
// If filename = "../../sensitive.txt"
// save_path = "uploads/../../sensitive.txt" = "../sensitive.txt"
```

### The Fix

```cpp
std::string UploadHandler::sanitizeFilename(const std::string& filename) {
    std::string safe_name;
    
    // Only allow: letters, digits, dots, hyphens, underscores
    for (size_t i = 0; i < filename.length(); ++i) {
        char c = filename[i];
        
        if ((c >= 'a' && c <= 'z') ||  // lowercase
            (c >= 'A' && c <= 'Z') ||  // uppercase
            (c >= '0' && c <= '9') ||  // digits
            c == '.' || c == '-' || c == '_') {
            safe_name += c;
        } else {
            safe_name += '_';  // Replace unsafe chars with underscore
        }
    }
    
    // Remove leading dots (prevents .htaccess attacks)
    while (!safe_name.empty() && safe_name[0] == '.') {
        safe_name = safe_name.substr(1);
    }
    
    // Ensure not empty
    if (safe_name.empty()) {
        safe_name = "unnamed_file";
    }
    
    // Prevent directory traversal
    // (already done by removing '/' above, but double-check)
    size_t pos;
    while ((pos = safe_name.find("..")) != std::string::npos) {
        safe_name.erase(pos, 2);
    }
    
    return safe_name;
}
```

### Examples

| Original | Sanitized |
|----------|-----------|
| `photo.jpg` | `photo.jpg` |
| `../../../etc/passwd` | `etcpasswd` |
| `my file.txt` | `my_file.txt` |
| `.htaccess` | `htaccess` |
| `../../../` | `unnamed_file` |
| `image<script>.png` | `imagescript.png` |

### Additional Protection: Unique Names

```cpp
std::string UploadHandler::generateUniqueFilename(const std::string& original) {
    std::string sanitized = sanitizeFilename(original);
    
    // Get file extension
    std::string extension;
    size_t dot_pos = sanitized.find_last_of('.');
    if (dot_pos != std::string::npos) {
        extension = sanitized.substr(dot_pos);
        sanitized = sanitized.substr(0, dot_pos);
    }
    
    // Add timestamp
    time_t now = time(NULL);
    std::ostringstream oss;
    oss << sanitized << "_" << now << extension;
    
    return oss.str();
}
```

**Example:** `photo.jpg` → `photo_1703012345.jpg`

**Benefits:**
- Prevents filename collisions
- Unique identifier
- Preserves original name and extension

---

## 📏 File Size Validation

### Why Limit?

```cpp
// 100MB upload = 100MB RAM
// 10 simultaneous uploads = 1GB RAM
// Server crash! 💥
```

### Implementation

```cpp
bool UploadHandler::validateFileSize(const UploadedFile& file) {
    size_t file_size = file.content.length();
    
    if (file_size == 0) {
        return false;  // Empty file
    }
    
    if (file_size > max_file_size) {
        return false;  // Too large
    }
    
    return true;
}
```

### Usage in Handler

```cpp
HttpResponse UploadHandler::handle(const HttpRequest& request) {
    // ... parse files ...
    
    for (size_t i = 0; i < files.size(); ++i) {
        if (!validateFileSize(files[i])) {
            std::ostringstream msg;
            msg << "File too large: " << files[i].filename
                << " (" << files[i].content.length() << " bytes, "
                << "limit is " << max_file_size << " bytes)";
            return HttpResponse::badRequest(msg.str());
        }
        
        // ... save file ...
    }
}
```

### HTTP 413 Payload Too Large

```cpp
if (file.content.size() > max_file_size) {
    return HttpResponse(413)
        .setBody("File too large")
        .setHeader("Retry-After", "3600");  // Try again in 1 hour
}
```

---

## 💾 Saving Files to Disk

### Implementation

```cpp
bool UploadHandler::saveFile(const std::string& path, 
                             const std::string& content) {
    // Open file in binary mode
    std::ofstream file(path.c_str(), 
                      std::ios::binary | std::ios::trunc);
    
    if (!file.is_open()) {
        return false;  // Can't open (permissions?)
    }
    
    // Write content
    file.write(content.c_str(), content.length());
    
    // Check for errors
    if (!file.good()) {
        file.close();
        return false;  // Write error (disk full?)
    }
    
    file.close();
    return true;
}
```

### Why Binary Mode?

```cpp
std::ios::binary | std::ios::trunc
         ^^^^^^^   ^^^^^^^^^^^^^^
         |         Clear file before writing
         No text conversion (preserve bytes exactly)
```

**Without binary:**
- Image/PDF corrupted
- Line ending conversion
- Binary data interpreted as text

### Error Handling

```cpp
std::string save_path = upload_dir + "/" + sanitizeFilename(file.filename);

if (!saveFile(save_path, file.content)) {
    // Possible reasons:
    // - No write permission on upload_dir
    // - Disk full
    // - Path too long
    // - Filesystem error
    
    return HttpResponse::internalServerError(
        "Failed to save file: " + file.filename
    );
}
```

---

## 📤 Multiple File Uploads

### HTML Form

```html
<form action="/upload" method="POST" enctype="multipart/form-data">
  <input type="file" name="file1">
  <input type="file" name="file2">
  <input type="file" name="file3">
  <input type="submit">
</form>
```

### HTTP Request

```http
POST /upload HTTP/1.1
Content-Type: multipart/form-data; boundary=----Boundary

------Boundary
Content-Disposition: form-data; name="file1"; filename="doc1.pdf"
Content-Type: application/pdf

[PDF data]
------Boundary
Content-Disposition: form-data; name="file2"; filename="img1.png"
Content-Type: image/png

[PNG data]
------Boundary
Content-Disposition: form-data; name="file3"; filename="data.json"
Content-Type: application/json

[JSON data]
------Boundary--
```

### Parsing Multiple Files

```cpp
std::vector<UploadedFile> files = parseMultipartFormData(body, boundary);

// files[0] = { field_name: "file1", filename: "doc1.pdf", ... }
// files[1] = { field_name: "file2", filename: "img1.png", ... }
// files[2] = { field_name: "file3", filename: "data.json", ... }
```

### Processing All Files

```cpp
HttpResponse UploadHandler::handle(const HttpRequest& request) {
    // ... extract boundary and parse ...
    
    std::vector<std::string> saved_files;
    
    for (size_t i = 0; i < files.size(); ++i) {
        // Validate
        if (!validateFileSize(files[i])) {
            return HttpResponse::badRequest("File too large: " + 
                                           files[i].filename);
        }
        
        // Sanitize filename
        std::string safe_name = sanitizeFilename(files[i].filename);
        std::string save_path = upload_dir + "/" + safe_name;
        
        // Save
        if (!saveFile(save_path, files[i].content)) {
            return HttpResponse::internalServerError(
                "Failed to save: " + safe_name
            );
        }
        
        saved_files.push_back(safe_name);
    }
    
    // Success response
    std::ostringstream body;
    body << "<h1>Upload Successful</h1><ul>";
    for (size_t i = 0; i < saved_files.size(); ++i) {
        body << "<li>" << saved_files[i] << "</li>";
    }
    body << "</ul>";
    
    HttpResponse response(201);  // Created
    response.setBody(body.str());
    response.setContentType("text/html");
    return response;
}
```

---

## 🔄 Complete Request Flow

### Putting It All Together

```cpp
HttpResponse UploadHandler::handle(const HttpRequest& request) {
    // 1. Check method
    if (request.getMethod() != "POST") {
        return HttpResponse::badRequest("Only POST allowed for uploads");
    }
    
    // 2. Get Content-Type header
    std::string content_type = request.getHeader("Content-Type");
    if (content_type.find("multipart/form-data") == std::string::npos) {
        return HttpResponse::badRequest(
            "Content-Type must be multipart/form-data"
        );
    }
    
    // 3. Extract boundary
    std::string boundary = extractBoundary(content_type);
    if (boundary.empty()) {
        return HttpResponse::badRequest("No boundary in Content-Type");
    }
    
    // 4. Parse multipart body
    std::vector<UploadedFile> files = parseMultipartFormData(
        request.getBody(),
        boundary
    );
    
    if (files.empty()) {
        return HttpResponse::badRequest("No files in request");
    }
    
    // 5. Process each file
    std::vector<std::string> saved_files;
    for (size_t i = 0; i < files.size(); ++i) {
        // Validate size
        if (!validateFileSize(files[i])) {
            return HttpResponse(413).setBody(
                "File too large: " + files[i].filename
            );
        }
        
        // Sanitize filename
        std::string safe_name = sanitizeFilename(files[i].filename);
        if (safe_name.empty()) {
            return HttpResponse::badRequest("Invalid filename");
        }
        
        // Save file
        std::string save_path = upload_dir + "/" + safe_name;
        if (!saveFile(save_path, files[i].content)) {
            return HttpResponse::internalServerError(
                "Failed to save: " + safe_name
            );
        }
        
        saved_files.push_back(safe_name);
    }
    
    // 6. Success response
    std::ostringstream body;
    body << "<h1>Upload Successful</h1>"
         << "<p>Uploaded " << saved_files.size() << " file(s):</p><ul>";
    for (size_t i = 0; i < saved_files.size(); ++i) {
        body << "<li><a href=\"/uploads/" << saved_files[i] << "\">"
             << saved_files[i] << "</a></li>";
    }
    body << "</ul>";
    
    HttpResponse response(201);  // Created
    response.setBody(body.str());
    response.setContentType("text/html");
    response.setHeader("Location", "/uploads/" + saved_files[0]);
    return response;
}
```

---

## 💡 Design Decisions

### Why Not Parse Incrementally?

**Current:** Load entire body, then parse
```cpp
std::string body = request.getBody();  // Entire body in memory
parseMultipartFormData(body, boundary);
```

**Alternative:** Stream parsing
```cpp
while (hasMoreData()) {
    chunk = readChunk();
    processChunk(chunk);
}
```

**Our choice:** Full body
- Simpler code
- File size is limited anyway (10MB)
- Good enough for this project
- Streaming is advanced (future enhancement)

### Why Vector for Files?

```cpp
std::vector<UploadedFile> files;
```

**Benefits:**
- Dynamic size (unknown number of files)
- Sequential access (process in order)
- C++98 compatible

**Alternative:** List
- Slower access
- More memory overhead
- Not needed here

### Why Struct for UploadedFile?

```cpp
struct UploadedFile {
    std::string field_name;
    std::string filename;
    std::string content_type;
    std::string content;
};
```

**Benefits:**
- Groups related data
- Clear and readable
- Easy to extend (add more fields)

**Alternative:** Multiple vectors
```cpp
std::vector<std::string> filenames;
std::vector<std::string> contents;
std::vector<std::string> types;
// Messy!
```

---

## 🔍 Real-World Scenarios

### Scenario 1: Photo Upload

**HTML:**
```html
<form action="/upload" method="POST" enctype="multipart/form-data">
  <input type="file" name="photo" accept="image/*">
  <button type="submit">Upload</button>
</form>
```

**Server Side:**
```cpp
UploadHandler handler("uploads/photos", 5 * 1024 * 1024);  // 5MB limit
HttpResponse response = handler.handle(request);
// Saves to: uploads/photos/photo_1703012345.jpg
```

### Scenario 2: Document Management

**Multiple Files:**
```html
<input type="file" name="documents" multiple>
```

**Result:**
```
uploads/
  ├── report_1703012345.pdf
  ├── data_1703012346.xlsx
  └── presentation_1703012347.pptx
```

### Scenario 3: Avatar Upload

**Special handling:**
```cpp
if (file.content_type.find("image/") != 0) {
    return HttpResponse::badRequest("Only images allowed for avatars");
}

if (file.content.size() > 1024 * 1024) {  // 1MB
    return HttpResponse::badRequest("Avatar must be under 1MB");
}

// Resize image (using external library)
std::string resized = resizeImage(file.content, 200, 200);
saveFile("avatars/" + user_id + ".png", resized);
```

---

## 🔍 Common Questions for Defense

**Q: Why not use URL encoding for uploads?**
- URL encoding is for text (converts binary to text)
- Inefficient (33% size increase)
- Multipart is standard for file uploads
- Supports multiple files naturally

**Q: What if filename is missing?**
```cpp
if (file.filename.empty()) {
    file.filename = "unnamed_" + file.field_name;
}
```

**Q: Can you upload without filename?**
```http
Content-Disposition: form-data; name="data"

Some text content
```
- Yes, it's valid (text field, not file)
- Check `filename` attribute to distinguish
- Our handler only processes parts with filenames

**Q: How to handle upload errors mid-upload?**
- Current: All-or-nothing (parse entire body first)
- Better: Streaming (save as you receive)
- Best: Resumable uploads (HTTP Range header)

**Q: What about virus scanning?**
```cpp
bool scanForVirus(const std::string& path) {
    // Use ClamAV or similar
    // Not implemented in our basic server
    return true;  // Assume safe
}

if (!scanForVirus(save_path)) {
    unlink(save_path.c_str());  // Delete file
    return HttpResponse::forbidden("File rejected by security scan");
}
```

**Q: Why not use Content-Transfer-Encoding?**
- HTTP doesn't need it (allows binary)
- Email uses it (must be text)
- Multipart can have it, but rare
- Our parser ignores it (assumes binary)

---

## 📚 Code References

### Files:
- `includes/UploadHandler.hpp` - Class definition
- `srcs/UploadHandler.cpp` - Implementation

### Key Methods:
```cpp
HttpResponse handle(const HttpRequest& request);
std::string extractBoundary(const std::string& ct);
std::vector<UploadedFile> parseMultipartFormData(...);
UploadedFile parsePart(const std::string& part);
std::string sanitizeFilename(const std::string& filename);
bool saveFile(const std::string& path, const std::string& content);
```

### Data Structures:
```cpp
struct UploadedFile {
    std::string field_name;
    std::string filename;
    std::string content_type;
    std::string content;
};
```

---

## 🧪 Testing Your Understanding

### Exercise 1: Parse Boundary
Extract boundary from:
1. `multipart/form-data; boundary=abc123`
2. `multipart/form-data; boundary="----WebKit"; charset=utf-8`
3. `application/json`

### Exercise 2: Sanitize Filenames
Sanitize these:
1. `../../../etc/passwd`
2. `my photo.jpg`
3. `.htaccess`
4. `file<script>.exe`

### Exercise 3: Multipart Parsing
Given this body (boundary = `----Bound`):
```
------Bound
Content-Disposition: form-data; name="file"; filename="test.txt"
Content-Type: text/plain

Hello World
------Bound--
```
What are the values of:
- `field_name`?
- `filename`?
- `content_type`?
- `content`?

---

**Continue to [Tutorial 06: Non-Blocking I/O Integration](06_Non_Blocking_IO.md) →**
