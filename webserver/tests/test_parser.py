#!/usr/bin/env python3
"""
Test script for HTTP request parser using raw sockets and telnet-style communication
"""

import socket
import time

HOST = 'localhost'
PORT = 8080

def send_request(request_data, description):
    """Send a raw HTTP request and receive response"""
    print(f"\n{'='*60}")
    print(f"Test: {description}")
    print(f"{'='*60}")
    print("Sending request:")
    print(request_data)
    print("-" * 60)
    
    try:
        # Create socket connection
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5.0)
        sock.connect((HOST, PORT))
        
        # Send request
        sock.sendall(request_data.encode('utf-8'))
        
        # Receive response
        response = b''
        while True:
            try:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                response += chunk
            except socket.timeout:
                break
        
        sock.close()
        
        print("Received response:")
        print(response.decode('utf-8', errors='replace'))
        return True
        
    except Exception as e:
        print(f"Error: {e}")
        return False

def test_get_request():
    """Test basic GET request"""
    request = (
        "GET /test.html HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: PythonTestClient/1.0\r\n"
        "Accept: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
    )
    send_request(request, "GET Request - Static File")

def test_get_with_query():
    """Test GET request with query string"""
    request = (
        "GET /api/search?q=test&limit=10 HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Connection: close\r\n"
        "\r\n"
    )
    send_request(request, "GET Request - With Query String")

def test_post_request():
    """Test POST request with body"""
    body = "name=John&email=john@example.com&message=Hello"
    request = (
        f"POST /api/submit HTTP/1.1\r\n"
        f"Host: localhost:8080\r\n"
        f"Content-Type: application/x-www-form-urlencoded\r\n"
        f"Content-Length: {len(body)}\r\n"
        f"Connection: close\r\n"
        f"\r\n"
        f"{body}"
    )
    send_request(request, "POST Request - Form Data")

def test_post_json():
    """Test POST request with JSON body"""
    body = '{"name": "Test", "value": 123, "active": true}'
    request = (
        f"POST /api/data HTTP/1.1\r\n"
        f"Host: localhost:8080\r\n"
        f"Content-Type: application/json\r\n"
        f"Content-Length: {len(body)}\r\n"
        f"Connection: close\r\n"
        f"\r\n"
        f"{body}"
    )
    send_request(request, "POST Request - JSON Data")

def test_delete_request():
    """Test DELETE request"""
    request = (
        "DELETE /api/items/123 HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Authorization: Bearer token123\r\n"
        "Connection: close\r\n"
        "\r\n"
    )
    send_request(request, "DELETE Request")

def test_invalid_method():
    """Test invalid HTTP method"""
    request = (
        "INVALID /test HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Connection: close\r\n"
        "\r\n"
    )
    send_request(request, "Invalid HTTP Method (should return 405)")

def test_malformed_request():
    """Test malformed request"""
    request = (
        "GET /test\r\n"  # Missing HTTP version
        "Host: localhost:8080\r\n"
        "\r\n"
    )
    send_request(request, "Malformed Request (should return 400)")

def test_directory_listing():
    """Test directory listing"""
    request = (
        "GET / HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Connection: close\r\n"
        "\r\n"
    )
    send_request(request, "Directory Listing (root)")

def test_404_not_found():
    """Test 404 error"""
    request = (
        "GET /nonexistent/file.html HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Connection: close\r\n"
        "\r\n"
    )
    send_request(request, "404 Not Found")

def test_large_headers():
    """Test request with many headers"""
    request = (
        "GET /test.html HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: TestClient/1.0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
        "Accept-Language: en-US,en;q=0.5\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "DNT: 1\r\n"
        "Connection: close\r\n"
        "Upgrade-Insecure-Requests: 1\r\n"
        "Cache-Control: max-age=0\r\n"
        "\r\n"
    )
    send_request(request, "Request with Multiple Headers")

def main():
    print("="*60)
    print("HTTP Request Parser Test Suite")
    print("="*60)
    print(f"Target: {HOST}:{PORT}")
    print("Make sure your webserver is running before executing tests!")
    print()
    
    input("Press Enter to start tests...")
    
    # Run all tests
    test_get_request()
    time.sleep(0.5)
    
    test_get_with_query()
    time.sleep(0.5)
    
    test_post_request()
    time.sleep(0.5)
    
    test_post_json()
    time.sleep(0.5)
    
    test_delete_request()
    time.sleep(0.5)
    
    test_directory_listing()
    time.sleep(0.5)
    
    test_404_not_found()
    time.sleep(0.5)
    
    test_large_headers()
    time.sleep(0.5)
    
    test_invalid_method()
    time.sleep(0.5)
    
    test_malformed_request()
    
    print("\n" + "="*60)
    print("All tests completed!")
    print("="*60)

if __name__ == "__main__":
    main()
