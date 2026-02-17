# CGI Implementation Guide for Webserv

## What is CGI?

CGI (Common Gateway Interface) is a standard interface for web servers to communicate with external programs. When a client requests a dynamic resource (like a PHP file), the server:

1. Recognizes the file type
2. Forks a child process
3. Executes the CGI interpreter (PHP, Python, etc.)
4. Sends request data to the CGI program
5. Captures the output
6. Sends it back to the client

---

## CGI Communication Flow

```
Client Browser
     ↓
HTTP Request: GET /cgi-bin/hello.php?name=John
     ↓
Webserv Server
     ↓
1. Recognizes .php → check cgi_pass .php /usr/bin/php-cgi
2. Creates pipes for input/output
3. Forks child process
4. Child: execve("/usr/bin/php-cgi", args, env)
5. Parent: reads output from pipe
6. Parent: collects all CGI output
     ↓
HTTP Response
     ↓
Client Browser
```

---

## Step-by-Step Implementation

### 1. Recognize CGI Request

In your request handler, check if the URL matches a CGI rule:

```cpp
bool is_cgi_request(const std::string& path, const LocationBlock& location) {
    for (const auto& cgi_rule : location.cgi_passes) {
        // cgi_rule.extension = ".php"
        // cgi_rule.interpreter = "/usr/bin/php-cgi"
        
        if (path.ends_with(cgi_rule.extension)) {
            return true;
        }
    }
    return false;
}
```

### 2. Parse CGI Requirements

Extract what the CGI needs:

```cpp
struct CGIRequest {
    std::string interpreter;        // /usr/bin/php-cgi
    std::string script_path;        // /var/www/html/cgi-bin/hello.php
    std::string query_string;       // name=John&age=30
    std::map<std::string, std::string> env_vars;
    std::string body;              // For POST requests
    size_t content_length;         // Size of body
};
```

### 3. Build Environment Variables

CGI programs read HTTP request data from environment variables:

```cpp
std::map<std::string, std::string> build_cgi_env(
    const HTTPRequest& request,
    const CGIRequest& cgi_req,
    const ServerBlock& server) {
    
    std::map<std::string, std::string> env;
    
    // Essential variables
    env["REQUEST_METHOD"] = request.method;  // GET, POST, DELETE
    env["SCRIPT_FILENAME"] = cgi_req.script_path;
    env["QUERY_STRING"] = cgi_req.query_string;
    
    // Server info
    env["SERVER_NAME"] = server.server_name;
    env["SERVER_PORT"] = std::to_string(server.port);
    env["SERVER_PROTOCOL"] = "HTTP/1.1";
    
    // Client info
    env["REMOTE_ADDR"] = client_ip;
    
    // For POST/PUT requests
    if (request.method == "POST" || request.method == "PUT") {
        env["CONTENT_TYPE"] = request.headers["Content-Type"];
        env["CONTENT_LENGTH"] = std::to_string(cgi_req.body.size());
    }
    
    // Request headers (prefix with HTTP_)
    for (const auto& [key, value] : request.headers) {
        std::string env_key = "HTTP_" + uppercase(key);
        env[env_key] = value;
    }
    
    // Working directory (for relative path file access)
    env["PWD"] = cgi_req.script_path.parent_path();
    
    return env;
}
```

### 4. Create Pipes for Communication

```cpp
int pipe_input[2];   // Server writes (request body), CGI reads
int pipe_output[2];  // CGI writes (response), Server reads

if (pipe(pipe_input) == -1) {
    perror("pipe_input");
    return false;
}
if (pipe(pipe_output) == -1) {
    perror("pipe_output");
    close(pipe_input[0]);
    close(pipe_input[1]);
    return false;
}
```

### 5. Fork and Execute

```cpp
pid_t pid = fork();

if (pid == -1) {
    // Fork failed
    perror("fork");
    close(pipe_input[0]);
    close(pipe_input[1]);
    close(pipe_output[0]);
    close(pipe_output[1]);
    return false;
}

if (pid == 0) {
    // === CHILD PROCESS ===
    
    // Close unused ends
    close(pipe_input[1]);    // Don't need to write to input
    close(pipe_output[0]);   // Don't need to read from output
    
    // Redirect stdin
    if (dup2(pipe_input[0], STDIN_FILENO) == -1) {
        perror("dup2 stdin");
        exit(1);
    }
    
    // Redirect stdout
    if (dup2(pipe_output[1], STDOUT_FILENO) == -1) {
        perror("dup2 stdout");
        exit(1);
    }
    
    // Close original fds (now duplicated)
    close(pipe_input[0]);
    close(pipe_output[1]);
    
    // Change to script directory (for relative paths)
    if (!cgi_req.script_dir.empty()) {
        chdir(cgi_req.script_dir.c_str());
    }
    
    // Convert environment map to char* array
    std::vector<char*> env_array;
    for (auto& [key, value] : env_vars) {
        std::string env_str = key + "=" + value;
        char* env_ptr = new char[env_str.length() + 1];
        strcpy(env_ptr, env_str.c_str());
        env_array.push_back(env_ptr);
    }
    env_array.push_back(NULL);  // Null-terminate
    
    // Execute CGI interpreter
    const char* argv[] = {
        interpreter.c_str(),  // e.g., "/usr/bin/php-cgi"
        script_path.c_str(),  // e.g., "/var/www/hello.php"
        NULL
    };
    
    execve(interpreter.c_str(), (char* const*)argv, (char* const*)&env_array[0]);
    
    // If execve returns, it failed
    perror("execve");
    exit(1);
    
} else {
    // === PARENT PROCESS ===
    
    // Close unused ends
    close(pipe_input[0]);    // Don't need to read from input
    close(pipe_output[1]);   // Don't need to write to output
    
    // Write request body if POST
    if (!cgi_req.body.empty()) {
        ssize_t bytes_written = 0;
        const char* data = cgi_req.body.c_str();
        size_t total = cgi_req.body.size();
        
        while (bytes_written < (ssize_t)total) {
            ssize_t ret = write(pipe_input[1], 
                               data + bytes_written, 
                               total - bytes_written);
            if (ret < 0) {
                perror("write to CGI stdin");
                break;
            }
            bytes_written += ret;
        }
    }
    
    // Close write end to signal EOF to CGI
    close(pipe_input[1]);
    
    // Read all CGI output
    std::string cgi_output;
    char buffer[4096];
    ssize_t bytes_read;
    
    while ((bytes_read = read(pipe_output[0], buffer, sizeof(buffer))) > 0) {
        cgi_output.append(buffer, bytes_read);
    }
    
    close(pipe_output[0]);
    
    // Wait for child process
    int status;
    waitpid(pid, &status, 0);
    
    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        if (exit_code != 0) {
            // CGI program failed
            return create_error_response(502, "Bad Gateway");
        }
    } else if (WIFSIGNALED(status)) {
        // CGI was killed by signal
        return create_error_response(502, "Bad Gateway");
    }
    
    // Process CGI output
    return process_cgi_output(cgi_output);
}
```

### 6. Parse CGI Output

CGI programs output HTTP headers followed by body:

```
Content-Type: text/html
Content-Length: 50

<html><body>Hello World</body></html>
```

```cpp
std::string process_cgi_output(const std::string& cgi_output) {
    // Find blank line separator (header/body boundary)
    size_t blank_line_pos = cgi_output.find("\r\n\r\n");
    if (blank_line_pos == std::string::npos) {
        blank_line_pos = cgi_output.find("\n\n");
        if (blank_line_pos == std::string::npos) {
            // Malformed CGI output
            return create_error_response(502, "Bad Gateway");
        }
        blank_line_pos += 2;  // Skip \n\n
    } else {
        blank_line_pos += 4;  // Skip \r\n\r\n
    }
    
    std::string headers = cgi_output.substr(0, blank_line_pos);
    std::string body = cgi_output.substr(blank_line_pos);
    
    // Parse headers from CGI
    std::map<std::string, std::string> cgi_headers;
    std::istringstream header_stream(headers);
    std::string line;
    while (std::getline(header_stream, line)) {
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            // Trim whitespace
            trim(value);
            cgi_headers[key] = value;
        }
    }
    
    // If no Content-Length header, use body size
    if (cgi_headers.find("Content-Length") == cgi_headers.end()) {
        cgi_headers["Content-Length"] = std::to_string(body.size());
    }
    
    // Build HTTP response
    std::string response = "HTTP/1.1 200 OK\r\n";
    for (const auto& [key, value] : cgi_headers) {
        response += key + ": " + value + "\r\n";
    }
    response += "\r\n";
    response += body;
    
    return response;
}
```

### 7. Handle Chunked Transfer from CGI

If CGI output uses chunked transfer encoding:

```cpp
std::string unchunk_data(const std::string& chunked) {
    std::string result;
    size_t pos = 0;
    
    while (pos < chunked.length()) {
        // Read chunk size line
        size_t crlf = chunked.find("\r\n", pos);
        if (crlf == std::string::npos) break;
        
        std::string size_line = chunked.substr(pos, crlf - pos);
        size_t chunk_size = std::stoul(size_line, nullptr, 16);
        
        if (chunk_size == 0) break;  // Last chunk
        
        pos = crlf + 2;  // Skip CRLF
        
        // Read chunk data
        result.append(chunked.substr(pos, chunk_size));
        
        pos += chunk_size + 2;  // Skip chunk data and trailing CRLF
    }
    
    return result;
}
```

---

## Testing Your CGI Implementation

### 1. Test with PHP

Create `/var/www/cgi-bin/test.php`:
```php
<?php
echo "Content-Type: text/html\r\n";
echo "Content-Length: 50\r\n";
echo "\r\n";
echo "<html><body>Hello from PHP CGI</body></html>";
?>
```

Request:
```bash
curl http://localhost:8080/cgi-bin/test.php
```

### 2. Test with Python

Create `/var/www/cgi-bin/test.py`:
```python
#!/usr/bin/env python3

import os
import sys

print("Content-Type: text/html")
print("Content-Length: 100")
print()
print("<html><body>")
print(f"<p>Hello from Python CGI</p>")
print(f"<p>Query: {os.environ.get('QUERY_STRING', '')}</p>")
print("</body></html>")
```

### 3. Test with Query String

```bash
curl "http://localhost:8080/cgi-bin/test.php?name=John&age=30"
```

### 4. Test with POST Data

```bash
curl -X POST -d "username=john&password=secret" \
     http://localhost:8080/cgi-bin/login.php
```

### 5. Test File Uploads via CGI

```bash
curl -F "file=@image.jpg" http://localhost:8080/cgi-bin/upload.php
```

---

## Common CGI Issues & Solutions

### Issue: CGI script outputs nothing
**Solution:** Check if script is executable (`chmod +x script.php`)

### Issue: "Command not found" errors
**Solution:** Check interpreter path. Use full path like `/usr/bin/python3` not just `python`

### Issue: CGI times out
**Solution:** Add timeout handling in your parent process after fork/exec

### Issue: Environment variables not passed
**Solution:** Make sure you null-terminate the env array: `env_array.push_back(NULL);`

### Issue: CGI output truncated
**Solution:** Handle multiple reads from pipe until EOF (use while loop)

### Issue: Child process zombie
**Solution:** Always call `waitpid()` to reap child process

---

## Configuration Example

```nginx
# Enable CGI for PHP files
server {
    listen 127.0.0.1:8080;
    server_name localhost;
    root /var/www/html;
    
    location /cgi-bin {
        allow_methods GET POST;
        cgi_pass .php /usr/bin/php-cgi;
        cgi_pass .py /usr/bin/python3;
    }
    
    # Direct to index.php
    location / {
        allow_methods GET POST;
        index index.php index.html;
    }
}
```

---

## Environment Variables Reference

These are the standard CGI environment variables. Your server should set them:

```
REQUEST_METHOD      - GET, POST, DELETE, PUT, HEAD, etc.
SCRIPT_FILENAME     - Full path to the CGI script being executed
QUERY_STRING        - Everything after ? in the URL
CONTENT_TYPE        - MIME type of request body (for POST/PUT)
CONTENT_LENGTH      - Size of request body in bytes
HTTP_ACCEPT         - Accepted content types (from Accept header)
HTTP_ACCEPT_ENCODING - Accepted encodings (from Accept-Encoding)
HTTP_ACCEPT_LANGUAGE - Accepted languages (from Accept-Language)
HTTP_CONNECTION     - Connection type (from Connection header)
HTTP_HOST           - Host and port (from Host header)
HTTP_REFERER        - Referring page (from Referer header)
HTTP_USER_AGENT     - Browser info (from User-Agent header)
HTTP_COOKIE         - Cookies (from Cookie header)
REMOTE_ADDR         - Client IP address
REMOTE_HOST         - Client hostname (if available)
SERVER_NAME         - Server hostname
SERVER_PORT         - Port number
SERVER_PROTOCOL     - HTTP/1.0 or HTTP/1.1
SERVER_SOFTWARE     - Server name/version
GATEWAY_INTERFACE   - CGI/1.1
PATH_INFO           - Extra path after script (if any)
PATH_TRANSLATED     - Translated PATH_INFO to filesystem
SCRIPT_NAME         - Script name/path
AUTH_TYPE           - Authentication type (if using auth)
REMOTE_USER         - Authenticated username (if using auth)
DOCUMENT_ROOT       - Root directory
HOME                - User home directory (inherited)
PATH                - System PATH (inherited)
```

---

## Performance Tips

1. **Reuse interpreters:** Cache PHP/Python processes if possible
2. **Set timeouts:** Kill hung CGI processes after N seconds
3. **Limit CGI processes:** Don't fork more than needed
4. **Monitor pipes:** Use select/poll for CGI I/O
5. **Clean up resources:** Always close fds and wait for children

---

## Debugging CGI Issues

Enable debugging in your code:

```cpp
void debug_cgi(const CGIRequest& cgi_req, 
               const std::map<std::string, std::string>& env) {
    std::cerr << "=== CGI DEBUG ===" << std::endl;
    std::cerr << "Interpreter: " << cgi_req.interpreter << std::endl;
    std::cerr << "Script: " << cgi_req.script_path << std::endl;
    std::cerr << "Query String: " << cgi_req.query_string << std::endl;
    std::cerr << "Environment Variables:" << std::endl;
    for (const auto& [key, value] : env) {
        std::cerr << "  " << key << " = " << value << std::endl;
    }
    std::cerr << "Body size: " << cgi_req.body.size() << std::endl;
}
```

Use strace to trace system calls:
```bash
strace -e trace=execve,fork,pipe,read,write ./webserv config.conf
```

---

Happy CGI coding! 🚀
