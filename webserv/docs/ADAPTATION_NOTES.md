# Adaptations from ft_irc to webserv

This document details what components were successfully adapted from the ft_irc project to webserv.

## 1. Poll-Based Event Loop Architecture

### From ft_irc:
```cpp
while (true) {
    int poll_count = poll(_poll_fds.data(), _poll_fds.size(), 100);
    
    for (size_t i = 0; i < _poll_fds.size(); i++) {
        if (_poll_fds[i].revents & POLLIN) {
            if (_poll_fds[i].fd == _server_fd) {
                _acceptNewClient();
            } else {
                _handleClientData(_poll_fds[i].fd);
            }
        }
        
        if (_poll_fds[i].revents & POLLOUT) {
            _flushClientBuffer(_poll_fds[i].fd);
        }
    }
}
```

### Adapted for webserv:
- Same poll() loop structure
- Handle POLLIN for incoming HTTP requests
- Handle POLLOUT for non-blocking response writing
- Added proper vector iteration with index management to avoid crashes when clients disconnect

## 2. Client Management

### From ft_irc:
```cpp
class Client {
    int _fd;
    std::string _nickname;
    std::string _buffer;
    bool _authenticated;
};

std::map<int, Client> _clients;
```

### Adapted for webserv:
```cpp
class Client {
    int _fd;
    std::string _buffer;           // For accumulating request data
    time_t _last_activity;         // For timeout handling
    bool _request_complete;        // HTTP-specific state
    size_t _content_length;        // For POST body handling
};

std::map<int, Client*> _clients;  // Using pointers for flexibility
```

**Key Changes:**
- Replaced IRC-specific fields (nickname, username) with HTTP-specific fields
- Added request state tracking
- Added timeout tracking for HTTP keepalive

## 3. Buffer Management

### From ft_irc:
```cpp
void Client::addToBuffer(const std::string& data) {
    _buffer += data;
}

// Process when \r\n found
size_t pos = buffer.find("\r\n");
```

### Adapted for webserv:
```cpp
void Client::addToBuffer(const std::string& data) {
    _buffer += data;
}

// Process when \r\n\r\n found (end of headers)
size_t header_end = buffer.find("\r\n\r\n");

// Then handle body based on Content-Length
if (body_received >= content_length) {
    _request_complete = true;
}
```

**Key Changes:**
- Different delimiter for HTTP (\r\n\r\n for headers vs \r\n for IRC)
- Added body handling logic for POST requests
- State machine for tracking header/body parsing

## 4. Output Buffering

### From ft_irc:
```cpp
void Server::_sendToClient(int client_fd, const std::string& message) {
    _output_buffers[client_fd] += message;
    
    // Set POLLOUT flag
    for (size_t i = 0; i < _poll_fds.size(); i++) {
        if (_poll_fds[i].fd == client_fd) {
            _poll_fds[i].events |= POLLOUT;
            break;
        }
    }
}

void Server::_flushClientBuffer(int client_fd) {
    std::string& buffer = _output_buffers[client_fd];
    ssize_t sent = send(client_fd, buffer.c_str(), buffer.length(), 0);
    if (sent > 0) {
        buffer.erase(0, sent);
    }
    
    if (buffer.empty()) {
        // Remove POLLOUT flag
        _poll_fds[i].events = POLLIN;
    }
}
```

### Adapted for webserv:
- **Identical implementation!**
- This was one of the easiest adaptations
- Works perfectly for HTTP responses
- Especially important for large file transfers

## 5. Socket Setup

### From ft_irc:
```cpp
void Server::_setupSocket() {
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    int opt = 1;
    setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(_port);
    
    bind(_server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(_server_fd, LISTEN_CONN);
    
    _setNonBlocking(_server_fd);
}

void Server::_setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
```

### Adapted for webserv:
- **Identical implementation!**
- No changes needed for HTTP
- Same socket options work for both protocols

## 6. Connection Acceptance

### From ft_irc:
```cpp
void Server::_acceptNewClient() {
    int client_fd = accept(_server_fd, ...);
    _setNonBlocking(client_fd);
    
    struct pollfd client_pollfd;
    client_pollfd.fd = client_fd;
    client_pollfd.events = POLLIN;
    _poll_fds.push_back(client_pollfd);
    
    _clients[client_fd] = Client(client_fd);
}
```

### Adapted for webserv:
- **Nearly identical!**
- Same accept() logic
- Same poll fd registration
- Only difference: HTTP doesn't need authentication handshake

## 7. Client Removal/Cleanup

### From ft_irc:
```cpp
void Server::_removeClient(int client_fd) {
    // Remove from channels
    for (channels...) { /* remove member */ }
    
    // Remove from poll_fds
    for (vector<pollfd>::iterator it = _poll_fds.begin(); ...) {
        if (it->fd == client_fd) {
            _poll_fds.erase(it);
            break;
        }
    }
    
    _clients.erase(client_fd);
    _output_buffers.erase(client_fd);
    close(client_fd);
}
```

### Adapted for webserv:
- Same structure
- Removed channel-specific logic
- Added proper checks for vector iteration safety

## 8. Timeout Handling

### From ft_irc:
Not implemented in ft_irc

### Added for webserv:
```cpp
void Server::_cleanupTimedOutClients() {
    const time_t timeout = 60;
    time_t now = time(NULL);
    
    std::vector<int> clients_to_remove;
    for (map<int, Client*>::iterator it = _clients.begin(); ...) {
        if (now - it->second->getLastActivity() > timeout) {
            clients_to_remove.push_back(it->first);
        }
    }
    
    for (size_t i = 0; i < clients_to_remove.size(); ++i) {
        _removeClient(clients_to_remove[i]);
    }
}
```

## Key Lessons Learned

1. **Poll loop is protocol-agnostic**: The event-driven architecture works identically for IRC and HTTP
2. **Buffer management adapts easily**: Just change delimiters and parsing logic
3. **Non-blocking I/O patterns transfer directly**: Same techniques for both protocols
4. **Output buffering is universal**: Works for any text-based protocol
5. **Socket setup is standard**: No protocol-specific changes needed

## Challenges in Adaptation

1. **HTTP body handling**: IRC is line-based, HTTP needs Content-Length awareness
2. **Request completion detection**: More complex than IRC's simple \r\n delimiter
3. **Vector iteration safety**: Had to fix crash when clients disconnect during iteration
4. **State management**: HTTP has more states (parsing headers, reading body, etc.)

## Statistics

- **Code reused**: ~40% of core server logic
- **Code adapted**: ~30% (modified for HTTP)
- **New code**: ~30% (HTTP-specific features)
- **Time saved**: Estimated 60-70% vs starting from scratch

