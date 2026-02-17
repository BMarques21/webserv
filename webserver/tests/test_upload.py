#!/usr/bin/env python3
"""
Test script for file upload functionality using curl-like multipart/form-data
"""

import socket
import os
import sys

HOST = 'localhost'
PORT = 8080

def create_multipart_body(filename, content, boundary):
    """Create a multipart/form-data body"""
    body = []
    body.append(f'--{boundary}')
    body.append(f'Content-Disposition: form-data; name="file"; filename="{filename}"')
    body.append('Content-Type: text/plain')
    body.append('')
    body.append(content)
    body.append(f'--{boundary}--')
    body.append('')
    
    return '\r\n'.join(body)

def test_file_upload(filename, content):
    """Test file upload"""
    boundary = '----WebKitFormBoundary7MA4YWxkTrZu0gW'
    body = create_multipart_body(filename, content, boundary)
    
    request = (
        f"POST /upload HTTP/1.1\r\n"
        f"Host: {HOST}:{PORT}\r\n"
        f"Content-Type: multipart/form-data; boundary={boundary}\r\n"
        f"Content-Length: {len(body)}\r\n"
        f"Connection: close\r\n"
        f"\r\n"
        f"{body}"
    )
    
    print(f"Uploading file: {filename}")
    print(f"Content size: {len(content)} bytes")
    print("-" * 60)
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10.0)
        sock.connect((HOST, PORT))
        
        sock.sendall(request.encode('utf-8'))
        
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
        
        print("Server response:")
        print(response.decode('utf-8', errors='replace'))
        return True
        
    except Exception as e:
        print(f"Error: {e}")
        return False

def test_multiple_files():
    """Test uploading multiple files"""
    boundary = '----WebKitFormBoundary7MA4YWxkTrZu0gW'
    
    # Create multipart body with multiple files
    body = []
    
    # File 1
    body.append(f'--{boundary}')
    body.append('Content-Disposition: form-data; name="file1"; filename="test1.txt"')
    body.append('Content-Type: text/plain')
    body.append('')
    body.append('This is the first test file')
    body.append('')
    
    # File 2
    body.append(f'--{boundary}')
    body.append('Content-Disposition: form-data; name="file2"; filename="test2.txt"')
    body.append('Content-Type: text/plain')
    body.append('')
    body.append('This is the second test file')
    body.append('')
    
    # End boundary
    body.append(f'--{boundary}--')
    body.append('')
    
    body_str = '\r\n'.join(body)
    
    request = (
        f"POST /upload HTTP/1.1\r\n"
        f"Host: {HOST}:{PORT}\r\n"
        f"Content-Type: multipart/form-data; boundary={boundary}\r\n"
        f"Content-Length: {len(body_str)}\r\n"
        f"Connection: close\r\n"
        f"\r\n"
        f"{body_str}"
    )
    
    print("Uploading multiple files...")
    print("-" * 60)
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10.0)
        sock.connect((HOST, PORT))
        
        sock.sendall(request.encode('utf-8'))
        
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
        
        print("Server response:")
        print(response.decode('utf-8', errors='replace'))
        return True
        
    except Exception as e:
        print(f"Error: {e}")
        return False

def main():
    print("="*60)
    print("File Upload Test Suite")
    print("="*60)
    print(f"Target: {HOST}:{PORT}")
    print("Make sure your webserver is running!")
    print()
    
    input("Press Enter to start tests...")
    
    # Test 1: Simple text file
    print("\n" + "="*60)
    print("Test 1: Upload simple text file")
    print("="*60)
    test_file_upload("hello.txt", "Hello, World!\nThis is a test file.")
    
    # Test 2: Larger file
    print("\n" + "="*60)
    print("Test 2: Upload larger file")
    print("="*60)
    large_content = "Line {}\n".format(1) * 100
    test_file_upload("large.txt", large_content)
    
    # Test 3: Multiple files
    print("\n" + "="*60)
    print("Test 3: Upload multiple files")
    print("="*60)
    test_multiple_files()
    
    print("\n" + "="*60)
    print("All upload tests completed!")
    print("Check the ./uploads directory for uploaded files")
    print("="*60)

if __name__ == "__main__":
    main()
