# Webserv Frontend-Backend Integration Documentation

## Table of Contents
1. [Overview](#overview)
2. [API Endpoints](#api-endpoints)
3. [Frontend Functions That Call Backend](#frontend-functions)
4. [Integration Points](#integration-points)
5. [Testing Guide](#testing-guide)
6. [Backend Implementation Examples](#backend-implementation-examples)

---

## Overview

The Webserv website is a **frontend testing interface** for your HTTP/1.1 server. It sends HTTP requests to your backend server and displays the responses. Here's how they communicate:

```
User Interface (HTML/CSS/JS)
         ↓ (HTTP Requests)
    app.js & chatbot.js
         ↓ (fetch API)
    Your Webserv Backend
         ↓ (HTTP Response)
    app.js processes response
         ↓
    Display results on page
```

---

## API Endpoints

These are the URLs that the frontend tries to connect to. **Your backend must implement these:**

### **1. GET /api/test**
- **Purpose:** Test basic GET request functionality
- **Method:** GET
- **Expected Response:** Any HTTP 200 response with body content
- **Frontend Location:** `app.js` line ~24 in `testMethod()` function
- **Called By:** Clicking "GET" button in HTTP Testing tab
- **Example Response:**
  ```
  Status: 200 OK
  Body: { "message": "GET request successful" }
  ```

### **2. POST /api/test**
- **Purpose:** Test POST request with JSON data
- **Method:** POST
- **Content-Type:** application/json
- **Request Body:**
  ```json
  {
    "data": "test",
    "timestamp": "2025-11-16T10:30:00.000Z"
  }
  ```
- **Expected Response:** HTTP 200 with confirmation
- **Frontend Location:** `app.js` line ~26-30 in `testMethod()` function
- **Called By:** Clicking "POST" button in HTTP Testing tab
- **Example Response:**
  ```
  Status: 200 OK
  Body: { "message": "POST received", "data": "test" }
  ```

### **3. DELETE /api/test/1**
- **Purpose:** Test DELETE request with resource ID
- **Method:** DELETE
- **Expected Response:** HTTP 200 or 204
- **Frontend Location:** `app.js` line ~24 in `testMethod()` function
- **Called By:** Clicking "DELETE" button in HTTP Testing tab
- **Example Response:**
  ```
  Status: 200 OK
  Body: { "message": "Resource deleted" }
  ```

### **4. POST /upload**
- **Purpose:** Handle file uploads
- **Method:** POST
- **Content-Type:** multipart/form-data
- **Request Body:** File data in FormData format
- **Expected Response:** HTTP 200 with file info
- **Frontend Location:** `app.js` line ~132-145 in `handleFileUpload()` function
- **Called By:** File upload in Upload tab
- **Example Response:**
  ```
  Status: 200 OK
  Body: { "message": "File uploaded", "filename": "image.jpg", "size": 2048 }
  ```

### **5. Custom Requests**
- **Purpose:** Allow users to test any method/URL combination
- **Methods:** GET, POST, DELETE, PUT, HEAD
- **URL:** Any path (from input field)
- **Frontend Location:** `app.js` line ~55-57 in `testCustomMethod()` function
- **Called By:** Dropdown + URL input + "Send Request" button
- **Example:** POST to `/api/custom` with any body

### **6. GET /api/metrics** (NEW)
- **Purpose:** Return server performance metrics
- **Method:** GET
- **Expected Response:** HTTP 200 with JSON metrics
- **Frontend Location:** `app.js` line ~195 in `fetchPerformanceMetrics()` function
- **Called By:** "Fetch Live Metrics" button in Features tab (Performance Benchmarks section)
- **Expected Response Format:**
  ```json
  {
    "connections": "10,000+",
    "uptime": "99.9%"
  }
  ```
- **Response Time:** Measured by frontend - displayed as "Average Response Time"
- **Notes:** 
  - The frontend automatically measures how long this request takes
  - `connections`: Maximum concurrent connections your server can handle (string format)
  - `uptime`: Server uptime percentage (string format, e.g., "99.9%")
  - Can include additional fields like `version`, `requests_per_second`, etc.

---

## Frontend Functions That Call Backend

### **Function 1: `testMethod(method, url)`**
**Location:** `app.js` lines 18-49

```javascript
async function testMethod(method, url) {
    try {
        const options = {
            method: method,
            headers: {}
        };

        if (method === 'POST') {
            options.body = JSON.stringify({ 
                data: 'test', 
                timestamp: new Date().toISOString() 
            });
            options.headers['Content-Type'] = 'application/json';
        }

        // THIS IS WHERE THE REQUEST IS SENT TO YOUR BACKEND
        const response = await fetch(url, options);
        const data = await response.text();

        displayResult({
            method,
            url,
            status: response.status,
            statusText: response.statusText,
            body: data,
            timestamp: new Date().toISOString()
        });
    } catch (error) {
        displayResult({
            method,
            url,
            error: error.message,
            timestamp: new Date().toISOString()
        });
    }
}
```

**What happens:**
1. Creates fetch options with HTTP method
2. For POST requests, adds JSON body with test data
3. Sends request using `fetch(url, options)`
4. Waits for response
5. Displays result on page

**Your Backend Must:**
- Listen on the specified URL
- Handle the HTTP method (GET, POST, DELETE, etc.)
- Return a valid HTTP response

---

### **Function 2: `testCustomMethod()`**
**Location:** `app.js` lines 51-57

```javascript
function testCustomMethod() {
    const method = document.getElementById('methodSelect').value;
    const url = document.getElementById('urlInput').value;
    testMethod(method, url);
}
```

**What happens:**
1. Gets method from dropdown (GET, POST, DELETE, PUT, HEAD)
2. Gets URL from text input
3. Calls `testMethod()` to send the request

**Your Backend Must:**
- Handle any HTTP method at any URL the user enters
- Return appropriate status codes and responses

---

### **Function 3: `handleFileUpload(event)`**
**Location:** `app.js` lines 107-171

```javascript
async function handleFileUpload(event) {
    const file = event.target.files[0];
    if (!file) return;

    const statusDiv = document.getElementById('uploadStatus');
    
    // Show loading animation
    statusDiv.innerHTML = `<div>Uploading ${file.name}...</div>`;

    const formData = new FormData();
    formData.append('file', file);

    try {
        // THIS IS WHERE THE FILE IS UPLOADED TO YOUR BACKEND
        const response = await fetch('/upload', {
            method: 'POST',
            body: formData
        });

        if (response.ok) {
            statusDiv.innerHTML = `✓ Upload successful! File: ${file.name}`;
            displayResult({
                method: 'POST',
                url: '/upload',
                status: response.status,
                statusText: 'OK',
                body: `File uploaded: ${file.name} (${(file.size / 1024).toFixed(2)} KB)`,
                timestamp: new Date().toISOString()
            });
        } else {
            throw new Error(`Upload failed with status ${response.status}`);
        }
    } catch (error) {
        statusDiv.innerHTML = `✗ Upload failed: ${error.message}`;
    }

    // Clear status after 5 seconds
    setTimeout(() => {
        statusDiv.innerHTML = '';
    }, 5000);
}
```

**What happens:**
1. Gets selected file
2. Creates FormData with file
3. Sends POST request to `/upload`
4. Shows success or error message
5. Displays result in test results section

**Your Backend Must:**
- Listen on `/upload` endpoint
- Accept POST requests with multipart/form-data
- Parse the file from the request
- Save or process the file
- Return HTTP 200 on success

---

## Integration Points

### **Point 1: HTTP Testing Tab**

**HTML Elements:**
```html
<!-- In index.html around line 350-380 -->
<div id="testing" class="tab-content">
    <button onclick="testMethod('GET', '/api/test')">GET</button>
    <button onclick="testMethod('POST', '/api/test')">POST</button>
    <button onclick="testMethod('DELETE', '/api/test/1')">DELETE</button>
    
    <select id="methodSelect">
        <option>GET</option>
        <option>POST</option>
        <option>DELETE</option>
        <option>PUT</option>
        <option>HEAD</option>
    </select>
    <input type="text" id="urlInput" placeholder="/api/endpoint">
    <button onclick="testCustomMethod()">Send Request</button>
    
    <div id="testResults"></div>
</div>
```

**How It Works:**
1. User clicks a button or enters custom request
2. JavaScript calls `testMethod()` or `testCustomMethod()`
3. `fetch()` sends HTTP request to your backend
4. Response is captured and displayed in `testResults` div

**Integration Checklist:**
- ✅ Backend listens on `/api/test` for GET, POST, DELETE
- ✅ Backend returns proper HTTP status codes
- ✅ Backend returns response body (text, JSON, HTML, etc.)
- ✅ Backend handles errors gracefully

---

### **Point 2: File Upload Tab**

**HTML Elements:**
```html
<!-- In index.html around line 390-410 -->
<div id="upload" class="tab-content">
    <div class="upload-zone" onclick="document.getElementById('fileInput').click()">
        <input type="file" id="fileInput" onchange="handleFileUpload(event)">
    </div>
    <div id="uploadStatus"></div>
</div>
```

**How It Works:**
1. User selects file
2. JavaScript calls `handleFileUpload(event)`
3. File is sent as FormData in POST request to `/upload`
4. Status shown in `uploadStatus` div

**Integration Checklist:**
- ✅ Backend listens on `/upload` endpoint
- ✅ Backend accepts multipart/form-data
- ✅ Backend extracts file from request
- ✅ Backend saves/processes file
- ✅ Backend returns HTTP 200 on success

---

## Testing Guide

### **Test 1: Basic GET Request**

**Step 1:** Start your backend server
```bash
./webserv  # or your server startup command
```

**Step 2:** Open the website in browser
```
file:///c:/Users/031br/OneDrive/Webserver/index.html
```
Or if served by your backend:
```
http://localhost:8080/index.html
```

**Step 3:** Go to "HTTP Testing" tab

**Step 4:** Click "GET" button

**Expected Result:**
- Status: 200 (or appropriate status code)
- Body: Response from `/api/test` GET endpoint
- Timestamp: When request was made

**If it fails:**
- Check backend is running
- Check `/api/test` endpoint exists
- Check backend is listening on correct port
- Check browser console for CORS errors

---

### **Test 2: POST Request with JSON**

**Step 1:** Go to "HTTP Testing" tab

**Step 2:** Click "POST" button

**Step 3:** Frontend automatically sends:
```json
{
  "data": "test",
  "timestamp": "2025-11-16T10:30:00.000Z"
}
```

**Expected Result:**
- Status: 200 (or appropriate status code)
- Body: Confirmation from backend
- Your backend should receive the JSON data

**If it fails:**
- Check backend accepts `application/json` content type
- Check backend parses JSON body correctly
- Check backend at `/api/test` handles POST

---

### **Test 3: Custom Request**

**Step 1:** Go to "HTTP Testing" tab

**Step 2:** 
- Select "POST" from dropdown
- Type `/custom/endpoint` in URL field
- Click "Send Request"

**Expected Result:**
- Request sent to custom endpoint
- Response displayed

**If it fails:**
- Check endpoint exists on backend
- Check HTTP method is supported

---

### **Test 4: File Upload**

**Step 1:** Go to "File Upload" tab

**Step 2:** Click upload zone or select file

**Step 3:** Choose any file from your computer

**Expected Result:**
- See uploading animation
- See ✓ success message
- File info shown in results

**If it fails:**
- Check `/upload` endpoint exists
- Check backend handles multipart/form-data
- Check file permissions (can backend write files?)
- Check max file size limits

---

## Backend Implementation Examples

### **Example 1: Simple GET Endpoint (C++)**

```cpp
// In your Webserv Server class
if (method == "GET" && path == "/api/test") {
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: application/json\r\n";
    response += "Content-Length: 40\r\n";
    response += "\r\n";
    response += "{\"message\": \"GET request successful\"}";
    
    send(client_socket, response.c_str(), response.length(), 0);
    return;
}
```

---

### **Example 2: POST Endpoint with JSON Body**

```cpp
if (method == "POST" && path == "/api/test") {
    // Parse JSON body
    std::string body = request.getBody();
    
    // Echo back the data
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: application/json\r\n";
    response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
    response += "\r\n";
    response += body;
    
    send(client_socket, response.c_str(), response.length(), 0);
    return;
}
```

---

### **Example 3: DELETE Endpoint**

```cpp
if (method == "DELETE" && path.find("/api/test/") == 0) {
    // Extract resource ID from path
    std::string id = path.substr(10); // "/api/test/" is 10 chars
    
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: application/json\r\n";
    response += "Content-Length: 35\r\n";
    response += "\r\n";
    response += "{\"message\": \"Resource deleted\"}";
    
    send(client_socket, response.c_str(), response.length(), 0);
    return;
}
```

---

### **Example 4: File Upload Endpoint**

```cpp
if (method == "POST" && path == "/upload") {
    // Parse multipart/form-data
    std::string boundary = request.getBoundary();
    std::string body = request.getBody();
    
    // Extract filename and file data
    std::string filename = request.extractFilename(body, boundary);
    std::string fileData = request.extractFileData(body, boundary);
    
    // Save file
    std::ofstream file("uploads/" + filename, std::ios::binary);
    file.write(fileData.c_str(), fileData.length());
    file.close();
    
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: application/json\r\n";
    response += "Content-Length: 50\r\n";
    response += "\r\n";
    response += "{\"message\": \"File uploaded\", \"filename\": \"" + filename + "\"}";
    
    send(client_socket, response.c_str(), response.length(), 0);
    return;
}
```

---

## Connection Checklist

Before testing, make sure:

- [ ] **Backend Server Running:** `./webserv` is executing
- [ ] **Correct Port:** Backend listening on correct port (usually 8080)
- [ ] **CORS Enabled:** If frontend and backend on different origins, enable CORS headers
- [ ] **Endpoints Implemented:**
  - [ ] `GET /api/test` → returns 200 with body
  - [ ] `POST /api/test` → accepts JSON, returns 200
  - [ ] `DELETE /api/test/1` → returns 200
  - [ ] `POST /upload` → accepts files, returns 200
- [ ] **Response Format:** All endpoints return valid HTTP responses with:
  - [ ] Status line (e.g., "HTTP/1.1 200 OK")
  - [ ] Headers (Content-Type, Content-Length, etc.)
  - [ ] Empty line
  - [ ] Body

---

## Troubleshooting

### **Problem: "Failed to fetch" error**
**Solution:** 
- Check backend server is running
- Check port is correct
- Check CORS headers if on different domain
- Check firewall isn't blocking port

### **Problem: Timeout waiting for response**
**Solution:**
- Check backend isn't hanging
- Check response has all required HTTP headers
- Check Content-Length is correct

### **Problem: File upload shows 0 bytes**
**Solution:**
- Check multipart/form-data parsing
- Check file data extraction logic
- Check file write permissions

### **Problem: Response shows but data is wrong format**
**Solution:**
- Check Content-Type header matches body
- Check response body is properly formatted
- Check character encoding

---

## Summary

**Frontend (this website):**
- Sends HTTP requests to your backend
- Displays responses to user
- Tests tabs, HTTP methods, file uploads

**Backend (your Webserv server):**
- Listen for requests from frontend
- Process requests (GET, POST, DELETE, etc.)
- Return proper HTTP responses
- Handle file uploads

**Integration is complete when:**
- Frontend sends request → Backend receives it
- Backend processes it → Frontend displays response
- User can test all HTTP methods and file uploads
