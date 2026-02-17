# Code Reference: Exact Lines Where Frontend Calls Backend

## File: app.js

### Section 1: testMethod() Function (Lines 18-49)
**This is the main function that sends HTTP requests to your backend**

```javascript
18: async function testMethod(method, url) {
19:     try {
20:         const options = {
21:             method: method,
22:             headers: {}
23:         };
24:
25:         if (method === 'POST') {
26:             options.body = JSON.stringify({ 
27:                 data: 'test', 
28:                 timestamp: new Date().toISOString() 
29:             });
30:             options.headers['Content-Type'] = 'application/json';
31:         }
32:
33:         // ⭐⭐⭐ HERE IS WHERE REQUEST IS SENT ⭐⭐⭐
34:         const response = await fetch(url, options);
35:         // ⭐⭐⭐ WAITING FOR YOUR BACKEND RESPONSE ⭐⭐⭐
36:
37:         const data = await response.text();
38:
39:         displayResult({
40:             method,
41:             url,
42:             status: response.status,
43:             statusText: response.statusText,
44:             body: data,
45:             timestamp: new Date().toISOString()
46:         });
47:     } catch (error) {
48:         displayResult({
49:             method,
50:             url,
51:             error: error.message,
52:             timestamp: new Date().toISOString()
53:         });
54:     }
55: }
```

**Key Line:** `34: const response = await fetch(url, options);`
- This sends the HTTP request to your backend
- `url` = where request goes (e.g., `/api/test`)
- `options.method` = HTTP method (GET, POST, DELETE, etc.)
- `options.body` = JSON data (only for POST)

**Your backend must:**
- Listen on the `url` path
- Accept the HTTP `method`
- Return a valid HTTP response
- Response is captured in `response` variable

---

### Section 2: Button Click Calls (From index.html)
**When user clicks buttons, these functions are called:**

```html
<!-- From index.html, around line 345-360 -->
<button onclick="testMethod('GET', '/api/test')">GET</button>
<!-- Calls: testMethod('GET', '/api/test') -->
<!-- Your backend receives: GET request to /api/test -->

<button onclick="testMethod('POST', '/api/test')">POST</button>
<!-- Calls: testMethod('POST', '/api/test') -->
<!-- Your backend receives: POST request to /api/test with JSON body -->

<button onclick="testMethod('DELETE', '/api/test/1')">DELETE</button>
<!-- Calls: testMethod('DELETE', '/api/test/1') -->
<!-- Your backend receives: DELETE request to /api/test/1 -->
```

---

### Section 3: testCustomMethod() (Lines 51-57)
**Gets values from form inputs and sends custom request**

```javascript
51: function testCustomMethod() {
52:     const method = document.getElementById('methodSelect').value;
53:     // Gets selected method from dropdown (GET, POST, DELETE, PUT, HEAD)
54:
55:     const url = document.getElementById('urlInput').value;
56:     // Gets URL from text input field (e.g., "/custom/endpoint")
57:
58:     testMethod(method, url);
59:     // Calls testMethod() which sends request to backend
60: }
```

**User enters:**
- Method: "POST"
- URL: "/my/custom/path"

**Calls:**
```javascript
testMethod('POST', '/my/custom/path')
```

**Your backend must:** Handle POST request to /my/custom/path

---

### Section 4: handleFileUpload() (Lines 107-171)
**Handles file uploads to backend**

```javascript
107: async function handleFileUpload(event) {
108:     const file = event.target.files[0];
109:     // Gets selected file from input
110:
111:     if (!file) return;
112:
113:     const statusDiv = document.getElementById('uploadStatus');
114:     statusDiv.innerHTML = `<div>Uploading ${file.name}...</div>`;
115:     // Shows loading message
116:
117:     const formData = new FormData();
118:     formData.append('file', file);
119:     // Creates FormData object with file
120:
121:     try {
122:         // ⭐⭐⭐ HERE FILE IS UPLOADED TO BACKEND ⭐⭐⭐
123:         const response = await fetch('/upload', {
124:             method: 'POST',
125:             body: formData
125:         });
126:         // ⭐⭐⭐ WAITING FOR BACKEND RESPONSE ⭐⭐⭐
127:
128:         if (response.ok) {
129:             statusDiv.innerHTML = `✓ Upload successful! File: ${file.name}`;
130:             // Shows success
131:
132:             displayResult({
133:                 method: 'POST',
134:                 url: '/upload',
135:                 status: response.status,
135:                 statusText: 'OK',
136:                 body: `File uploaded: ${file.name}`,
137:                 timestamp: new Date().toISOString()
138:             });
139:         } else {
140:             throw new Error(`Upload failed with status ${response.status}`);
141:         }
142:     } catch (error) {
143:         statusDiv.innerHTML = `✗ Upload failed: ${error.message}`;
143:         // Shows error
144:     }
145:
146:     setTimeout(() => {
147:         statusDiv.innerHTML = '';
148:     }, 5000);
149:     // Clears message after 5 seconds
150: }
```

**Key Line:** `123: const response = await fetch('/upload', { method: 'POST', body: formData });`
- Sends file to `/upload` endpoint
- Uses multipart/form-data format
- `formData` contains the file

**Your backend must:**
- Listen on `/upload` endpoint
- Accept POST requests
- Parse multipart/form-data
- Extract file from request
- Save file
- Return HTTP 200 response

---

### Section 5: displayResult() (Lines 59-106)
**Shows results from backend responses**

```javascript
59: function displayResult(result) {
60:     const resultsDiv = document.getElementById('testResults');
61:     // Gets results container div
62:
63:     const noResults = resultsDiv.querySelector('.no-results');
64:     if (noResults) {
65:         noResults.remove();
66:     }
67:     // Removes "no results" placeholder
68:
69:     const resultCard = document.createElement('div');
70:     resultCard.className = 'result-card';
71:
72:     let statusBadge = '';
73:     if (result.status) {
74:         const statusClass = result.status >= 200 && result.status < 300 ? 
75:             'status-success' : 'status-error';
76:         statusBadge = `<span class="status-badge ${statusClass}">
77:             ${result.status} ${result.statusText}</span>`;
78:     }
79:     // Creates color-coded status (green=success, red=error)
80:
81:     resultCard.innerHTML = `
82:         <div style="...">
83:             <strong>${result.method}</strong>
84:             <code>${result.url}</code>
85:         </div>
86:         ${statusBadge}
87:         ${result.error ? 
88:             `<div>Error: ${result.error}</div>` :
89:             `<div>Response Body: ${result.body}</div>`
90:         }
91:         <p>${new Date(result.timestamp).toLocaleString()}</p>
92:     `;
93:     // Creates HTML card showing:
94:     // - HTTP method (GET, POST, DELETE)
94:     // - URL tested
95:     // - Status code and text
96:     // - Error or response body
97:     // - Timestamp
98:
99:     resultsDiv.insertBefore(resultCard, resultsDiv.firstChild);
100:     // Adds card to top of results
101:
102:     while (resultsDiv.children.length > 5) {
103:         resultsDiv.removeChild(resultsDiv.lastChild);
103:     }
104:     // Keeps only last 5 results
105: }
```

**This function receives:**
- `result.status` = HTTP status code from backend (200, 404, 500, etc.)
- `result.statusText` = Status text (OK, Not Found, etc.)
- `result.body` = Response body from backend
- `result.error` = Error message if request failed

---

## HTML Integration Points

### Point 1: HTTP Testing Tab (index.html ~350-380)
```html
<div id="testing" class="tab-content">
    <!-- Button that calls testMethod -->
    <button class="btn btn-primary" 
            onclick="testMethod('GET', '/api/test')">
        GET
    </button>
    
    <!-- Input for custom requests -->
    <select id="methodSelect">
        <option>GET</option>
        <option>POST</option>
        <option>DELETE</option>
    </select>
    
    <input type="text" id="urlInput" placeholder="/api/endpoint">
    
    <button class="btn btn-primary" 
            onclick="testCustomMethod()">
        Send Request
    </button>
    
    <!-- Results displayed here -->
    <div id="testResults"></div>
</div>
```

---

### Point 2: File Upload Tab (index.html ~390-410)
```html
<div id="upload" class="tab-content">
    <div class="upload-zone" 
         onclick="document.getElementById('fileInput').click()">
        <input type="file" 
               id="fileInput" 
               onchange="handleFileUpload(event)">
        <!-- When file selected, handleFileUpload() is called -->
    </div>
    
    <!-- Upload status shown here -->
    <div id="uploadStatus"></div>
</div>
```

---

## Summary: What Your Backend Needs to Handle

| Frontend Call | Backend Must Handle |
|---------------|-------------------|
| `testMethod('GET', '/api/test')` | GET request to `/api/test` → return response |
| `testMethod('POST', '/api/test')` | POST request to `/api/test` with JSON body → return response |
| `testMethod('DELETE', '/api/test/1')` | DELETE request to `/api/test/1` → return response |
| `testMethod(method, customUrl)` | Any HTTP method to any URL → return response |
| `handleFileUpload(file)` | POST request to `/upload` with file → return response |
| `fetchPerformanceMetrics()` | GET request to `/api/metrics` → return JSON with metrics |

---

## Section 6: fetchPerformanceMetrics() Function (NEW)
**Fetches performance metrics from backend and updates benchmarks display**

```javascript
// Fetch performance metrics from backend
async function fetchPerformanceMetrics() {
    try {
        const startTime = performance.now();
        
        // Call backend metrics endpoint
        const response = await fetch('/api/metrics', {
            method: 'GET'
        });

        const endTime = performance.now();
        const responseTime = endTime - startTime;

        if (response.ok) {
            const data = await response.json();
            
            // Update UI with real data
            document.getElementById('metric-connections').textContent = data.connections || '10,000+';
            document.getElementById('metric-response').textContent = Math.round(responseTime) + 'ms';
            document.getElementById('metric-uptime').textContent = data.uptime || '99.9%';
            
            // Show success message
            displayResult({...});
        }
    } catch (error) {
        displayResult({...});
    }
}
```

**Key Line:** `const response = await fetch('/api/metrics', { method: 'GET' });`
- Calls `/api/metrics` endpoint on your backend
- Automatically measures response time
- Updates Performance Benchmarks display with live data
- Called by: "Fetch Live Metrics" button in Features tab

---

## Response Format Your Backend Must Return

**Minimum valid HTTP response:**

```
HTTP/1.1 200 OK
Content-Type: text/plain
Content-Length: 13

Hello Backend
```

Or for JSON:

```
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 40

{"message": "Request received successfully"}
```

**For /api/metrics endpoint specifically:**

```
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 50

{"connections": "10,000+", "uptime": "99.9%"}
```

**Required elements:**
1. Status line: `HTTP/1.1 [code] [text]`
2. At least one header: `Content-Type` (required), `Content-Length` (required)
3. Blank line: `\r\n\r\n` (required)
4. Response body (can be empty)

---

## Testing Checklist

Before saying integration is complete:

- [ ] Backend starts without errors
- [ ] Frontend can reach backend (no connection errors)
- [ ] Click "GET" button → Shows status 200 and response
- [ ] Click "POST" button → Shows status 200 and response
- [ ] Click "DELETE" button → Shows status 200 and response
- [ ] Enter custom URL/method → Works correctly
- [ ] Upload a file → Shows success message
- [ ] Click "Fetch Live Metrics" button → Shows live response time and metrics
- [ ] All responses properly formatted with HTTP headers
- [ ] Frontend displays response body correctly
