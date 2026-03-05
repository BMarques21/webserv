# WebServer - HTTP Components

C++98 implementation of HTTP request parser, response generator, static file handler, and file upload system for a non-blocking webserver project.

## 📁 Project Structure

```
webserver/
├── includes/              # Header files
│   ├── HttpRequest.hpp
│   ├── HttpResponse.hpp
│   ├── StaticFileHandler.hpp
│   └── UploadHandler.hpp
│
├── srcs/                  # Source files
│   ├── HttpRequest.cpp
│   ├── HttpResponse.cpp
│   ├── StaticFileHandler.cpp
│   └── UploadHandler.cpp
│
├── tests/                 # Test files
│   ├── test_http.cpp
│   ├── test_parser.py
│   └── test_upload.py
│
├── docs/                  # Documentation
│   ├── README.md
│   ├── QUICK_REFERENCE.md
│   ├── IMPLEMENTATION_SUMMARY.md
│   ├── PROJECT_CHECKLIST.md
│   └── example_integration.cpp
│
├── www/                   # Static web files
│   ├── index.html
│   ├── upload.html
│   ├── test.html
│   ├── style.css
│   └── script.js
│
├── uploads/               # Upload directory
│
├── Makefile              # Build configuration
├── build.sh              # Unix build script
├── build.ps1             # Windows build script
└── .gitignore            # Git ignore rules
```

## 🚀 Quick Start

### Build

**Linux/Unix:**
```bash
make
# or
./build.sh
```

**Windows:**
```powershell
.\build.ps1
```

### Test

```bash
./test_http                    # Run unit tests
python3 tests/test_parser.py   # Test with raw sockets
python3 tests/test_upload.py   # Test file uploads
```

## ✨ Features

### HTTP Request Parser
- ✅ GET, POST, DELETE methods
- ✅ Header parsing (case-insensitive)
- ✅ Query string extraction
- ✅ Request body parsing
- ✅ Multipart/form-data support
- ✅ Error validation with proper HTTP codes

### HTTP Response Generator
- ✅ Correct HTTP status codes
- ✅ Header management
- ✅ Default error pages
- ✅ Helper methods for common responses

### Static File Handler
- ✅ 25+ MIME types
- ✅ Directory listing
- ✅ Default file support
- ✅ Path security (prevents ../ attacks)

### Upload Handler
- ✅ Multipart/form-data parsing
- ✅ Multiple file support
- ✅ Size validation
- ✅ Filename sanitization

## 📚 Documentation

See the `docs/` directory for detailed documentation:

- **README.md** - Complete API documentation
- **QUICK_REFERENCE.md** - Code examples and quick guide
- **IMPLEMENTATION_SUMMARY.md** - Overview of all components
- **PROJECT_CHECKLIST.md** - Track your progress
- **example_integration.cpp** - Integration with poll()-based server

## 🧪 Testing

### Unit Tests
```bash
./test_http
```

### HTTP Parser Tests
```bash
python3 tests/test_parser.py
```

### Upload Tests
```bash
python3 tests/test_upload.py
```

### Manual Testing
```bash
# With telnet
telnet localhost 8080
GET /test.html HTTP/1.1
Host: localhost
[Press Enter twice]

# With curl
curl http://localhost:8080/test.html
curl -F "file=@test.txt" http://localhost:8080/upload
```

## 🔧 Requirements

- C++ compiler with C++98 support (g++, clang++)
- Make (optional)
- Python 3 (for test scripts)

## 📝 License

Educational project - Free to use and modify

## 🤝 Contributing

This is part of a webserver project. Contributions welcome!

---

For detailed documentation, see `docs/README.md`
