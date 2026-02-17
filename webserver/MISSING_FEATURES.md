# Subject Requirements Analysis - Missing Features

## Overview
Based on reading the complete `webserverSubject.pdf`, here's what's **MISSING** from the current implementation:

---

## MANDATORY FEATURES (Section IV.1)

### ✅ IMPLEMENTED:
- [x] Configuration file support (webserv.conf)
- [x] Non-blocking server (poll-based event loop)
- [x] HTTP status codes (200, 201, 404, 405, 413)
- [x] Default error pages (404.html, 500.html)
- [x] GET method
- [x] POST method (file upload)
- [x] DELETE method
- [x] PUT method (bonus, working)
- [x] HEAD method (bonus, working)
- [x] Static website serving
- [x] File uploads
- [x] Request body size limit (client_max_body_size)
- [x] Basic configuration parsing
- [x] Non-blocking I/O with poll()
- [x] Single poll() for all I/O operations

### ❌ MISSING - CRITICAL:

#### 2. **Multiple Port Support** (Required - MANDATORY)
   - **Subject Reference**: Section IV.3 - "Define all the interface:port pairs on which your server will listen to"
   - **Subject Reference**: IV.1 - "Your server must be able to listen to multiple ports to deliver different content"
   - **Current Implementation**: Only listens on 127.0.0.1:8080
   - **What's Missing**:
     - Config file parsing for multiple `server` blocks
     - Support for binding to different ports
     - Support for different host/interface addresses
     - Routing requests to correct server based on port/host
   - **Implementation Needed**: Parse webserv.conf to extract all `server` blocks and bind to all specified ports

#### 3. **CGI Execution** (Required - MANDATORY)
   - **Subject Reference**: Section IV.3 - "Execution of CGI, based on file extension (for example .php)"
   - **Subject Statement**: "You must provide configuration files and default files to test and demonstrate that every feature works"
   - **Current Status**: No CGI implementation
   - **What's Required**:
     - Parse `cgi_extension` from config (e.g., `.php`, `.py`)
     - Use `fork()` to spawn CGI process
     - Set up pipes for CGI stdin/stdout
     - Pass full request details as environment variables (REQUEST_METHOD, QUERY_STRING, CONTENT_LENGTH, etc.)
     - Handle chunked request bodies (un-chunk before passing to CGI)
     - Handle CGI output (support variable content-length or EOF marking)
     - Set correct working directory for CGI execution
   - **Critical Note**: Subject says fork() can ONLY be used for CGI - "You can't use fork for anything other than CGI"

#### 4. **HTTP Redirection** (Required - MANDATORY)
   - **Subject Reference**: Section IV.3 - "HTTP redirection" as a URL/route configuration option
   - **Current Status**: No redirection implementation
   - **What's Required**:
     - Support `redirect` directive in location blocks
     - Implement 301 (Moved Permanently) and 302 (Found) responses
     - Location header with redirect target
     - Example config:
       ```
       location /old-path {
           redirect /new-path;
       }
       ```

#### 5. **HTTP Method Restrictions per Route** (Required - MANDATORY)
   - **Subject Reference**: Section IV.3 - "List of accepted HTTP methods for the route"
   - **Current Status**: Partially implemented (allowed_methods in config)
   - **What's Missing**: 
     - Verify HTTP method against allowed_methods for that route
     - Return 405 (Method Not Allowed) if method not in allowed list
     - This should be enforced from config, not hardcoded

#### 6. **Directory Root Mapping** (Required - MANDATORY)
   - **Subject Reference**: Section IV.3 - "Directory where the requested file should be located (e.g., if URL /kapouet is rooted to /tmp/www, URL /kapouet/pouic/toto/pouet will search for /tmp/www/pouic/toto/pouet)"
   - **Current Status**: Partially working but needs verification
   - **What's Required**: Properly map URL routes to disk locations based on location root directives

#### 7. **Directory Listing Toggle** (Required - MANDATORY)
   - **Subject Reference**: Section IV.3 - "Enabling or disabling directory listing"
   - **Current Status**: Hardcoded behavior (some locations have autoindex on/off in config, but may not be enforced)
   - **What's Missing**: Properly parse and enforce `autoindex on/off` from config

#### 8. **Default Index File per Route** (Required - MANDATORY)
   - **Subject Reference**: Section IV.3 - "Default file to serve when the requested resource is a directory"
   - **Current Status**: Partially implemented (index.html hardcoded)
   - **What's Missing**: 
     - Parse `index` directive from config
     - Support multiple index files (e.g., `index index.html index.htm`)
     - Apply per-location, not globally

#### 9. **Upload Directory Configuration** (Required - MANDATORY)
   - **Subject Reference**: Section IV.3 - "Uploading files from the clients to the server is authorized, and storage location is provided"
   - **Current Status**: Uses hardcoded `uploads/` directory
   - **What's Missing**: 
     - Parse `upload_path` from config
     - Store uploaded files to configured location
     - Support different upload paths per location

#### 10. **No Hanging Requests** (Required - MANDATORY)
   - **Subject Reference**: IV.1 - "A request to your server should never hang indefinitely"
   - **Current Status**: Unknown - needs timeout implementation
   - **What's Missing**: Request timeout mechanism

#### 11. **Default Configuration File Path** (Required)
   - **Subject Reference**: IV.1 - "Your program must use a configuration file, provided as an argument on the command line, or available in a default path"
   - **Current Status**: Requires argument, no default path
   - **What's Missing**: If no config argument provided, load from default path (e.g., `./webserv.conf` or `/etc/webserv/webserv.conf`)

---

## CONFIGURATION FILE REQUIREMENTS (Section IV.3)

Current `webserv.conf` has basic structure but missing proper parsing/enforcement of:

1. ✅ `listen` directive (port number)
2. ❌ `host` directive (interface/IP to bind to) - needs implementation
3. ❌ `server_name` directive (for virtual hosts if implementing) 
4. ✅ `client_max_body_size` - implemented
5. ✅ `error_page` - basic support
6. ❌ `location` blocks with full parsing:
   - ❌ `root` directive (per-location)
   - ❌ `index` directive
   - ❌ `autoindex` on/off
   - ❌ `allowed_methods`
   - ❌ `redirect` directive (MISSING)
   - ❌ `upload_path` directive
   - ❌ `cgi_extension` directive

---

## BONUS FEATURES (Section V)

### Optional but could implement:
- [ ] Cookie and session management
- [ ] Multiple CGI types

---

## PRIORITY FIX ORDER

### **MUST HAVE (to pass evaluation):**
1. **Multiple port support** - Section IV.1 explicitly requires this
2. **CGI execution** - Section IV.3 explicitly requires this
3. **HTTP redirection** - Section IV.3 explicitly requires this
4. **Proper configuration parsing** - All routes and their settings must be read from config

### **SHOULD HAVE (to ensure robustness):**
5. Request timeouts (no hanging requests)
6. Proper default config file path
7. Full route-specific configuration enforcement

### **NICE TO HAVE:**
8. Bonus features (cookies, session management)

---

## VERIFICATION NOTES

From subject requirements:
- "You must provide configuration files and default files to test and demonstrate that every feature works during the evaluation"
- "Resilience is key. Your server must remain operational at all times"
- "Do not test with only one program. Write your tests in a more suitable language, such as Python or Golang"

---

## CONFIGURATION FILE EXAMPLE NEEDED

Need to create working test configurations that demonstrate:
1. Multiple servers on different ports
2. CGI execution with proper setup
3. Redirects
4. All route configurations working

Current `webserv.conf` is a skeleton but actual features referenced aren't implemented yet.
