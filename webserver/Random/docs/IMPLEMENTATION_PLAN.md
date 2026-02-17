# Webserv - Implementation Plan

## Phase 1: Core Server (✅ Complete)
- [x] Socket setup and management
- [x] Non-blocking I/O with poll()
- [x] Client connection handling
- [x] Request parsing (basic)
- [x] Response generation
- [x] Output buffering
- [x] Timeout handling

## Phase 2: HTTP Protocol Compliance (🔄 In Progress)
- [x] GET method implementation
- [ ] POST method with body parsing
- [ ] DELETE method
- [ ] Handle all required headers
- [ ] Implement chunked transfer encoding
- [ ] Request body size limits
- [ ] Keep-alive connections

## Phase 3: File Operations
- [x] Static file serving
- [x] Content-Type detection
- [x] Custom error pages (partial)
- [ ] Directory listing (autoindex)
- [ ] File uploads (POST)
- [ ] DELETE file operation
- [ ] Range requests (bonus)

## Phase 4: Configuration
- [x] Default configuration
- [ ] Parse configuration file (NGINX-style)
- [ ] Multiple server blocks
- [ ] Location blocks with rules
- [ ] Error page configuration
- [ ] Client body size limits
- [ ] CGI configuration per location

## Phase 5: CGI Support
- [x] Basic CGI execution framework
- [ ] Environment variable setup
- [ ] Non-blocking CGI I/O with poll()
- [ ] POST data to CGI stdin
- [ ] Parse CGI output
- [ ] CGI timeout handling
- [ ] Multiple CGI interpreters (.php, .py, etc.)

## Phase 6: Advanced Features
- [ ] Multiple ports/interfaces
- [ ] Virtual host support
- [ ] Redirections (301/302)
- [ ] HTTP/1.0 compatibility
- [ ] Proper MIME type handling
- [ ] Request validation

## Phase 7: Error Handling & Robustness
- [x] Basic error responses
- [ ] All HTTP error codes
- [ ] Custom error pages per location
- [ ] Graceful degradation
- [ ] Memory leak prevention
- [ ] Signal handling (SIGPIPE, SIGINT, etc.)

## Phase 8: Testing & Optimization
- [x] Basic curl tests
- [x] Automated test script
- [ ] Browser compatibility testing
- [ ] Stress testing (siege, ab)
- [ ] Memory leak testing (valgrind)
- [ ] Compare with NGINX behavior
- [ ] Performance optimization

## Bonus Features
- [ ] Cookie support
- [ ] Session management
- [ ] Multiple CGI types
- [ ] WebSocket support (if time permits)

## Code Quality
- [x] C++98 compliance
- [x] Proper header guards
- [x] Class organization
- [ ] Comprehensive error messages
- [ ] Code documentation
- [ ] Style consistency (42 norm or similar)

