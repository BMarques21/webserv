#!/bin/bash
# Build script for Linux/Unix/WSL

echo "Building WebServer HTTP Components..."

# Check if g++ is available
if ! command -v g++ &> /dev/null; then
    echo "Error: g++ not found. Please install g++."
    exit 1
fi

# Compile source files
echo "Compiling source files..."

SOURCES="srcs/HttpRequest.cpp srcs/HttpResponse.cpp srcs/StaticFileHandler.cpp srcs/UploadHandler.cpp tests/test_http.cpp"
CXXFLAGS="-Wall -Wextra -Werror -std=c++98 -Iincludes"

# Create objs directory
mkdir -p objs

# Clean old build
rm -f test_http objs/*.o

# Compile
g++ $CXXFLAGS -o test_http $SOURCES

if [ $? -eq 0 ]; then
    echo ""
    echo "Build successful!"
    echo "Executable: ./test_http"
    
    # Create directories if they don't exist
    mkdir -p www
    mkdir -p uploads
    
    echo ""
    echo "To run tests: ./test_http"
    echo "To run Python tests: python3 tests/test_parser.py"
else
    echo ""
    echo "Build failed!"
    exit 1
fi
