# Quick Integration Reference

## Where Frontend Calls Backend

### 1. **GET Request**
```javascript
// app.js line ~24
fetch('/api/test', { method: 'GET' })
```
**Your backend must respond to:** GET /api/test

---

### 2. **POST Request**
```javascript
// app.js line ~26-30
fetch('/api/test', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ data: 'test', timestamp: '...' })
})
```
**Your backend must respond to:** POST /api/test with JSON body

---

### 3. **DELETE Request**
```javascript
// app.js line ~24
fetch('/api/test/1', { method: 'DELETE' })
```
**Your backend must respond to:** DELETE /api/test/1

---

### 4. **File Upload**
```javascript
// app.js line ~145
const formData = new FormData();
formData.append('file', file);
fetch('/upload', {
    method: 'POST',
    body: formData
})
```
**Your backend must respond to:** POST /upload with file data

---

### 5. **Custom Requests**
```javascript
// app.js line ~51-57
// User selects method and URL, then:
fetch(userURL, { method: userMethod })
```
**Your backend must:** Handle any method/URL the user enters

---

### 6. **Performance Metrics** (NEW)
```javascript
// app.js line ~195
fetch('/api/metrics', { method: 'GET' })
```
**Your backend must respond to:** GET /api/metrics with JSON metrics

Response format:
```json
{
    "connections": "10,000+",
    "uptime": "99.9%"
}
```
**Called by:** "Fetch Live Metrics" button in Features tab
**Frontend also measures:** Response time automatically

---

## Minimal Backend Response Format

All responses must be valid HTTP:

```
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 35

{"message": "Success here"}
```

**Required parts:**
1. Status line: `HTTP/1.1 200 OK`
2. Headers: `Content-Type`, `Content-Length`, etc.
3. Blank line: `\r\n\r\n`
4. Body: Response data

---

## Testing Workflow

1. **Start backend:**
   ```bash
   ./webserv
   ```

2. **Open website:**
   ```
   http://localhost:8080/index.html
   (or file:// if serving separately)
   ```

3. **Go to "HTTP Testing" tab**

4. **Click buttons to test:**
   - GET → sends request to /api/test
   - POST → sends JSON to /api/test
   - DELETE → sends to /api/test/1
   - Custom → sends to user-entered URL

5. **Check results:**
   - Success: Shows status 200 and response body
   - Error: Shows red error message

---

## Frontend Files That Matter

| File | Contains | Purpose |
|------|----------|---------|
| `index.html` | UI structure | Buttons, forms, display areas |
| `app.js` | **`testMethod()` function** | **Sends requests to backend** |
| `app.js` | **`handleFileUpload()` function** | **Uploads files to backend** |
| `app.js` | **`fetchPerformanceMetrics()` function** | **Gets metrics from backend** |
| `styles.css` | Styling | Colors, layout |
| `chatbot.js` | Chatbot | Not related to backend |

---

## Backend Files You Need to Edit

In your Webserv C++ code, add handlers for:

1. **GET /api/test** - Return test response
2. **POST /api/test** - Handle JSON body and return response
3. **DELETE /api/test/1** - Handle deletion and return response
4. **POST /upload** - Handle multipart/form-data file upload
5. **GET /api/metrics** - Return JSON with metrics:
   ```json
   {
       "connections": "10,000+",
       "uptime": "99.9%"
   }
   ```

---

## Common Issues & Fixes

| Issue | Fix |
|-------|-----|
| "Failed to fetch" | Backend not running |
| Timeout | Backend not responding |
| Wrong response format | Check HTTP headers and status line |
| File upload 0 bytes | Check multipart parsing |
| CORS error | Add proper CORS headers in backend response |

---

## HTTP Response Headers You Need

```
HTTP/1.1 200 OK
Content-Type: application/json (or text/plain, etc.)
Content-Length: 35
Connection: keep-alive

{"your": "response body"}
```

All these are required for the frontend to receive data correctly.
