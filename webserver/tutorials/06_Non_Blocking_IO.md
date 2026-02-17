# Tutorial 06: Non-Blocking I/O Integration

## 📖 What You'll Learn

- Why non-blocking I/O matters
- poll() system call
- Event-driven architecture
- Integrating HTTP parser with poll()
- Managing client state
- Connection lifecycle

---

## 🎯 The Goal

**Blocking (Bad):**
```cpp
// Server stuck waiting for one client!
std::string request = read(socket);  // ⏸️ Blocks until data arrives
HttpRequest req;
req.parse(request);
```

**Non-Blocking (Good):**
```cpp
// Server handles many clients simultaneously!
poll(sockets, num_sockets, 0);  // Check which sockets are ready
for (each ready socket) {
    // Only read from sockets that have data
    std::string chunk = read_available(socket);
    clients[socket].request.parse(chunk);  // Incremental parsing
}
```

---

## 🏗️ Why Non-Blocking?

### The Problem with Blocking I/O

**Scenario:** 3 clients connect

```cpp
// Client 1 connects
int client1 = accept(server_socket);  // ✓ Got connection

// Read from client 1
std::string data = read(client1);  // ⏸️ BLOCKS waiting for data

// Meanwhile:
// - Client 2 is trying to connect → WAITING
// - Client 3 is trying to send data → WAITING
// Server is stuck on client 1!
```

**Result:**
- Only 1 client served at a time
- Other clients timeout
- Bad user experience
- Doesn't scale

### The Solution: Non-Blocking I/O

```cpp
// Server loop
while (true) {
    // Check ALL sockets (server + clients) for activity
    poll(all_sockets, num_sockets, timeout);
    
    // Server socket ready? → New connection
    if (server_socket_ready) {
        accept_new_client();
    }
    
    // Client socket ready? → Data to read
    for (each ready client) {
        read_and_process(client);
    }
}
```

**Benefits:**
- Handle multiple clients simultaneously
- No blocking (responsive server)
- Efficient (only process ready sockets)
- Scalable (100s of clients)

---

## 🔌 The poll() System Call

### What is poll()?

**Purpose:** Monitor multiple file descriptors (sockets) for events

```cpp
#include <poll.h>

int poll(struct pollfd *fds, nfds_t nfds, int timeout);
```

**Parameters:**
- `fds`: Array of pollfd structures (one per socket)
- `nfds`: Number of file descriptors to monitor
- `timeout`: Milliseconds to wait (-1 = forever, 0 = immediate)

**Returns:** Number of ready file descriptors

### pollfd Structure

```cpp
struct pollfd {
    int fd;         // File descriptor (socket)
    short events;   // Events to monitor
    short revents;  // Events that occurred
};
```

**Events:**
| Event | Meaning |
|-------|---------|
| `POLLIN` | Data available to read |
| `POLLOUT` | Ready to write |
| `POLLERR` | Error condition |
| `POLLHUP` | Hang up (client disconnected) |

### Basic Example

```cpp
// Setup
struct pollfd fds[2];

// Monitor server socket (for new connections)
fds[0].fd = server_socket;
fds[0].events = POLLIN;  // Watch for incoming connections

// Monitor client socket (for data)
fds[1].fd = client_socket;
fds[1].events = POLLIN;  // Watch for incoming data

// Poll (wait 1 second)
int ready = poll(fds, 2, 1000);

if (ready > 0) {
    // Check server socket
    if (fds[0].revents & POLLIN) {
        // New connection available
        int new_client = accept(server_socket, NULL, NULL);
    }
    
    // Check client socket
    if (fds[1].revents & POLLIN) {
        // Data available to read
        char buffer[1024];
        int bytes = recv(client_socket, buffer, 1024, 0);
    }
}
```

---

## 🏗️ Event-Driven Architecture

### Server Structure

```cpp
class WebServer {
private:
    int server_socket;
    std::vector<struct pollfd> poll_fds;         // All sockets to monitor
    std::map<int, ClientConnection> clients;     // Client state
    
    void acceptNewConnection();
    void handleClientData(int client_fd);
    void removeClient(int client_fd);
    
public:
    void run();  // Main event loop
};
```

### Client State Management

```cpp
struct ClientConnection {
    int socket_fd;
    HttpRequest request;           // Incremental parser
    bool request_complete;
    std::string response_buffer;   // Data to send
    size_t response_sent;          // Bytes already sent
};
```

**Why track state?**
- Request may arrive in chunks
- Response may need multiple send() calls
- Need to remember progress

---

## 🔄 Event Loop

### Main Loop Structure

```cpp
void WebServer::run() {
    while (true) {
        // 1. Poll all sockets
        int ready = poll(&poll_fds[0], poll_fds.size(), -1);
        
        if (ready < 0) {
            // Error
            break;
        }
        
        // 2. Check each socket
        for (size_t i = 0; i < poll_fds.size(); ++i) {
            if (poll_fds[i].revents == 0) {
                continue;  // No events
            }
            
            int fd = poll_fds[i].fd;
            
            // 3. Server socket → new connection
            if (fd == server_socket && (poll_fds[i].revents & POLLIN)) {
                acceptNewConnection();
            }
            // 4. Client socket → data or disconnect
            else if (poll_fds[i].revents & POLLIN) {
                handleClientData(fd);
            }
            // 5. Error or hangup → remove client
            else if (poll_fds[i].revents & (POLLERR | POLLHUP)) {
                removeClient(fd);
            }
        }
    }
}
```

### Accepting Connections

```cpp
void WebServer::acceptNewConnection() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    // Accept new connection
    int client_fd = accept(server_socket, 
                          (struct sockaddr*)&client_addr, 
                          &addr_len);
    
    if (client_fd < 0) {
        return;  // Accept failed
    }
    
    // Set non-blocking mode
    fcntl(client_fd, F_SETFL, O_NONBLOCK);
    
    // Add to poll array
    struct pollfd pfd;
    pfd.fd = client_fd;
    pfd.events = POLLIN;  // Watch for data
    pfd.revents = 0;
    poll_fds.push_back(pfd);
    
    // Initialize client state
    ClientConnection client;
    client.socket_fd = client_fd;
    client.request_complete = false;
    client.response_sent = 0;
    clients[client_fd] = client;
    
    std::cout << "New client: " << client_fd << std::endl;
}
```

---

## 📖 Incremental Parsing Integration

### Why Incremental?

**HTTP Request might arrive in chunks:**

**Chunk 1:**
```
GET /index.html HTTP/1.1\r\n
Host: localhost\r\n
```

**Chunk 2:**
```
User-Agent: Mozilla/5.0\r\n
\r\n
```

**Our parser handles this!**

### Handling Client Data

```cpp
void WebServer::handleClientData(int client_fd) {
    ClientConnection& client = clients[client_fd];
    
    // Read available data
    char buffer[4096];
    int bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
    
    if (bytes_read <= 0) {
        // Connection closed or error
        removeClient(client_fd);
        return;
    }
    
    // Parse chunk incrementally
    std::string chunk(buffer, bytes_read);
    bool complete = client.request.parse(chunk);
    
    if (complete) {
        // Request is complete! Process it.
        processRequest(client_fd);
    }
    
    // If not complete, just wait for more data
    // (next poll() iteration will read more)
}
```

### Processing Complete Request

```cpp
void WebServer::processRequest(int client_fd) {
    ClientConnection& client = clients[client_fd];
    HttpRequest& request = client.request;
    
    // Route request
    HttpResponse response;
    
    if (request.getMethod() == "GET" || 
        request.getMethod() == "DELETE") {
        // Serve static file
        StaticFileHandler handler("/var/www");
        response = handler.handle(request);
    }
    else if (request.getMethod() == "POST") {
        // Handle upload
        UploadHandler handler("uploads", 10 * 1024 * 1024);
        response = handler.handle(request);
    }
    else {
        response = HttpResponse::badRequest("Method not supported");
    }
    
    // Build response
    client.response_buffer = response.build();
    client.response_sent = 0;
    
    // Change poll event to POLLOUT (ready to send)
    for (size_t i = 0; i < poll_fds.size(); ++i) {
        if (poll_fds[i].fd == client_fd) {
            poll_fds[i].events = POLLOUT;  // Watch for write-ready
            break;
        }
    }
}
```

### Sending Response

```cpp
void WebServer::sendResponse(int client_fd) {
    ClientConnection& client = clients[client_fd];
    
    // How much left to send?
    size_t remaining = client.response_buffer.length() - client.response_sent;
    
    if (remaining == 0) {
        // All sent! Close connection.
        removeClient(client_fd);
        return;
    }
    
    // Send chunk
    const char* data = client.response_buffer.c_str() + client.response_sent;
    int bytes_sent = send(client_fd, data, remaining, 0);
    
    if (bytes_sent < 0) {
        // Error
        removeClient(client_fd);
        return;
    }
    
    client.response_sent += bytes_sent;
    
    // If all sent, close connection
    if (client.response_sent >= client.response_buffer.length()) {
        removeClient(client_fd);
    }
}
```

---

## 🔄 Complete Event Loop with Send

### Enhanced Loop

```cpp
void WebServer::run() {
    while (true) {
        int ready = poll(&poll_fds[0], poll_fds.size(), -1);
        
        for (size_t i = 0; i < poll_fds.size(); ++i) {
            if (poll_fds[i].revents == 0) continue;
            
            int fd = poll_fds[i].fd;
            
            // New connection
            if (fd == server_socket && (poll_fds[i].revents & POLLIN)) {
                acceptNewConnection();
            }
            // Read request
            else if (poll_fds[i].revents & POLLIN) {
                handleClientData(fd);
            }
            // Send response
            else if (poll_fds[i].revents & POLLOUT) {
                sendResponse(fd);
            }
            // Error
            else if (poll_fds[i].revents & (POLLERR | POLLHUP)) {
                removeClient(fd);
            }
        }
    }
}
```

---

## 📊 Connection Lifecycle

### State Diagram

```
[New Connection]
       ↓
   POLLIN (read)
       ↓
   Receive chunks → Parse incrementally
       ↓
   Request complete?
       ↓
   Process request
       ↓
   POLLOUT (write)
       ↓
   Send response chunks
       ↓
   All sent?
       ↓
   [Close Connection]
```

### Detailed Example

**Client sends:**
```http
GET /index.html HTTP/1.1\r\n
Host: localhost\r\n
\r\n
```

**Server timeline:**

1. **poll() returns** - Server socket has POLLIN
2. **accept()** - New client (fd = 5)
3. **Add to poll_fds** - Monitor fd 5 for POLLIN
4. **poll() returns** - fd 5 has POLLIN
5. **recv()** - Read chunk 1: "GET /index.html HTTP/1.1\r\nHost: localhost\r\n"
6. **parse()** - Not complete yet (no \r\n\r\n)
7. **poll() returns** - fd 5 has POLLIN again
8. **recv()** - Read chunk 2: "\r\n"
9. **parse()** - Complete! (found \r\n\r\n)
10. **processRequest()** - Build response
11. **Change to POLLOUT** - Ready to send
12. **poll() returns** - fd 5 has POLLOUT
13. **send()** - Send response (all at once or in chunks)
14. **Close** - Remove client

---

## 💡 Design Decisions

### Why poll() instead of select()?

| Feature | poll() | select() |
|---------|--------|----------|
| Max FDs | No limit | 1024 (FD_SETSIZE) |
| API | Modern, clean | Old, messy |
| Performance | O(n) | O(n) |
| Availability | POSIX | Everywhere |

**Our choice:** poll() (cleaner, no limit)

**Note:** epoll() (Linux) or kqueue() (BSD) are faster for 1000s of connections, but poll() is sufficient for this project.

### Why Not Threads?

**One thread per client:**
```cpp
for (each connection) {
    create_thread(handle_client);  // 100 clients = 100 threads
}
```

**Problems:**
- Memory overhead (1MB+ per thread)
- Context switching overhead
- Synchronization complexity (mutexes)
- Doesn't scale

**Event loop is better:**
- Single thread
- Efficient
- Simple
- No synchronization needed

### Why Track response_sent?

**Problem:** send() might not send everything

```cpp
// Response is 10KB
std::string response = ...;  // 10,000 bytes

// send() might only send 5KB
int sent = send(fd, response.c_str(), response.length(), 0);
// sent = 5000 (not 10000!)
```

**Solution:** Track progress
```cpp
client.response_sent = 0;
while (client.response_sent < client.response_buffer.length()) {
    int sent = send(fd, 
                   client.response_buffer.c_str() + client.response_sent,
                   remaining, 
                   0);
    client.response_sent += sent;
}
```

---

## 🔍 Real-World Scenarios

### Scenario 1: Slow Client

**Client has slow network:**
```
Request arrives in small chunks:
  Chunk 1: "GET /index"
  Chunk 2: ".html HT"
  Chunk 3: "TP/1.1\r\n"
  Chunk 4: "\r\n"
```

**Server handles it:**
- poll() wakes up 4 times
- parse() called 4 times (incremental)
- Complete on chunk 4

**No blocking!** Other clients are served between chunks.

### Scenario 2: Multiple Simultaneous Uploads

**3 clients upload at the same time:**

```
poll() returns:
  - Client 1 (fd=5): POLLIN → read chunk, parse
  - Client 2 (fd=6): POLLIN → read chunk, parse
  - Client 3 (fd=7): POLLIN → read chunk, parse

All progress simultaneously!
```

### Scenario 3: Client Disconnects Mid-Request

```
Client sends: "GET /index.html HTTP/1.1\r\nHost: loc"
Then disconnects (closes connection)
```

**Server detects:**
```cpp
int bytes = recv(client_fd, buffer, sizeof(buffer), 0);
if (bytes == 0) {
    // Client closed connection
    removeClient(client_fd);
}
```

---

## 🔍 Common Questions for Defense

**Q: What happens if parse() is called multiple times on complete request?**
```cpp
bool complete = request.parse(chunk);
if (complete) {
    // Set flag to prevent re-parsing
    client.request_complete = true;
}
```
- Our state machine handles it (stays in COMPLETE state)
- Should only call once per request

**Q: Can send() fail?**
- Yes! Client might disconnect during send
- Check return value
- Handle EAGAIN (try again later)

**Q: Why non-blocking mode for client sockets?**
```cpp
fcntl(client_fd, F_SETFL, O_NONBLOCK);
```
- recv() returns immediately if no data (not block)
- send() returns immediately if can't send all
- Essential for event loop

**Q: What if client sends huge request?**
- Parser tracks total bytes received
- Can limit max request size
- Return 413 Payload Too Large

**Q: How to implement keep-alive?**
```http
Connection: keep-alive
```
- Don't close connection after response
- Reset client state (parse new request)
- Set timeout (close if idle too long)

**Q: What's the timeout parameter in poll() for?**
```cpp
poll(fds, nfds, 1000);  // Wait 1 second
```
- -1: Wait forever (until event)
- 0: Return immediately (check and return)
- >0: Wait up to N milliseconds
- Our choice: -1 (event-driven, not time-driven)

---

## 📚 Code References

### Files:
- `example_integration.cpp` - Full event loop example
- `includes/HttpRequest.hpp` - Incremental parser

### Key Functions:
```cpp
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
int fcntl(int fd, int cmd, ...);  // Set non-blocking
```

### Constants:
```cpp
POLLIN   // Data to read
POLLOUT  // Ready to write
POLLERR  // Error
POLLHUP  // Hangup
```

---

## 🧪 Testing Your Understanding

### Exercise 1: Event Classification
What event (POLLIN/POLLOUT/POLLERR) for:
1. New client trying to connect?
2. Client sent data?
3. Server ready to send response?
4. Client closed connection?

### Exercise 2: State Transitions
Track client state through:
```
1. Client connects
2. Sends "GET /"
3. Sends "HTTP/1.1\r\n\r\n"
4. Server processes
5. Server sends response
6. Connection closes
```

What's the value of `request_complete` and `poll event` at each step?

### Exercise 3: Debug Scenario
Client sends 5KB request but recv() only reads 1KB at a time. How many poll() iterations to receive complete request?

---

**Continue to [Tutorial 07: Architecture & Design Patterns](07_Architecture.md) →**
