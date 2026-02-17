# WebServer Project - Complete Tutorial Series

Welcome to the comprehensive tutorial series for building an HTTP webserver from scratch in C++98.

## 📚 Tutorial Index

### Part 1: HTTP Fundamentals
- **[01_HTTP_Protocol_Basics.md](01_HTTP_Protocol_Basics.md)** - Understanding HTTP/1.1
  - What is HTTP?
  - Request/Response cycle
  - Status codes
  - Headers and body

### Part 2: Request Parsing
- **[02_HTTP_Request_Parser.md](02_HTTP_Request_Parser.md)** - Building the request parser
  - Why we need a parser
  - Parsing the request line
  - Parsing headers
  - Parsing the body
  - Incremental parsing for non-blocking I/O

### Part 3: Response Generation
- **[03_HTTP_Response_Generator.md](03_HTTP_Response_Generator.md)** - Creating HTTP responses
  - Response structure
  - Status codes explained
  - Header management
  - Content-Length calculation
  - Default error pages

### Part 4: Static File Serving
- **[04_Static_File_Handler.md](04_Static_File_Handler.md)** - Serving files from disk
  - File I/O in C++
  - MIME types and why they matter
  - Directory listing
  - Security considerations (path traversal)
  - Cross-platform file operations

### Part 5: File Uploads
- **[05_File_Upload_Handler.md](05_File_Upload_Handler.md)** - Handling multipart/form-data
  - Understanding multipart/form-data
  - Parsing boundaries
  - Extracting file content
  - Saving files securely
  - Size validation

### Part 6: Non-Blocking I/O
- **[06_NonBlocking_IO_Poll.md](06_NonBlocking_IO_Poll.md)** - poll() and event-driven servers
  - Blocking vs Non-blocking
  - Why use poll()?
  - Event-driven architecture
  - Managing client connections
  - Integration patterns

### Part 7: Project Architecture
- **[07_Project_Architecture.md](07_Project_Architecture.md)** - Overall design decisions
  - Why we separated components
  - Class design principles
  - C++98 constraints
  - Error handling strategy
  - Testing approach

### Part 8: Advanced Topics
- **[08_Advanced_Topics.md](08_Advanced_Topics.md)** - Beyond the basics
  - CGI execution
  - Configuration file parsing
  - Chunked transfer encoding
  - Keep-alive connections
  - Virtual hosts

## 🎯 How to Use These Tutorials

1. **Read in order** - Each tutorial builds on previous concepts
2. **Follow along with code** - Look at the actual implementation in `srcs/` and `includes/`
3. **Try the examples** - Each tutorial has practical examples
4. **Understand the "why"** - We explain not just how, but why we made each decision

## 🔍 Quick Reference

- Stuck on HTTP basics? → Start with Tutorial 01
- Need to explain the parser? → Tutorial 02
- Questions about MIME types? → Tutorial 04
- How does poll() work? → Tutorial 06

## 💡 For Your Defense/Evaluation

Each tutorial includes:
- ✅ **Theory** - Concepts you need to explain
- ✅ **Implementation** - How we coded it
- ✅ **Trade-offs** - Why we chose this approach
- ✅ **Common Questions** - What evaluators might ask

## 📖 Additional Resources

- `docs/README.md` - Complete API documentation
- `docs/QUICK_REFERENCE.md` - Code snippets and examples
- `docs/example_integration.cpp` - How to integrate everything

---

Start with **[01_HTTP_Protocol_Basics.md](01_HTTP_Protocol_Basics.md)** →
