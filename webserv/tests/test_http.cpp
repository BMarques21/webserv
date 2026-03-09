#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "StaticFileHandler.hpp"
#include "UploadHandler.hpp"
#include <iostream>
#include <sstream>
#include <cstring>

// Simple test program to demonstrate the HTTP components
void testHttpRequestParser() {
    std::cout << "=== Testing HTTP Request Parser ===" << std::endl;
    
    // Test GET request
    const char* get_request = 
        "GET /index.html?param=value HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: TestClient/1.0\r\n"
        "Accept: text/html\r\n"
        "\r\n";
    
    HttpRequest req1;
    req1.parse(get_request, strlen(get_request));
    
    std::cout << "GET Request:" << std::endl;
    std::cout << "  Method: " << req1.getMethodString() << std::endl;
    std::cout << "  URI: " << req1.getUri() << std::endl;
    std::cout << "  Query: " << req1.getQueryString() << std::endl;
    std::cout << "  Version: " << req1.getHttpVersion() << std::endl;
    std::cout << "  Host: " << req1.getHeader("Host") << std::endl;
    std::cout << "  Complete: " << (req1.isComplete() ? "Yes" : "No") << std::endl;
    std::cout << std::endl;
    
    // Test POST request with body
    const char* post_request = 
        "POST /api/upload HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 27\r\n"
        "\r\n"
        "name=test&message=hello";
    
    HttpRequest req2;
    req2.parse(post_request, strlen(post_request));
    
    std::cout << "POST Request:" << std::endl;
    std::cout << "  Method: " << req2.getMethodString() << std::endl;
    std::cout << "  URI: " << req2.getUri() << std::endl;
    std::cout << "  Content-Length: " << req2.getContentLength() << std::endl;
    std::cout << "  Body: " << req2.getBody() << std::endl;
    std::cout << "  Complete: " << (req2.isComplete() ? "Yes" : "No") << std::endl;
    std::cout << std::endl;
    
    // Test DELETE request
    const char* delete_request = 
        "DELETE /files/test.txt HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "\r\n";
    
    HttpRequest req3;
    req3.parse(delete_request, strlen(delete_request));
    
    std::cout << "DELETE Request:" << std::endl;
    std::cout << "  Method: " << req3.getMethodString() << std::endl;
    std::cout << "  URI: " << req3.getUri() << std::endl;
    std::cout << "  Complete: " << (req3.isComplete() ? "Yes" : "No") << std::endl;
    std::cout << std::endl;
}

void testHttpResponse() {
    std::cout << "=== Testing HTTP Response Generator ===" << std::endl;
    
    // Test OK response
    HttpResponse resp1 = HttpResponse::ok("<h1>Hello World</h1>", "text/html");
    std::cout << "OK Response:" << std::endl;
    std::cout << resp1.build() << std::endl;
    
    // Test 404 response
    HttpResponse resp2 = HttpResponse::notFound();
    std::cout << "404 Response:" << std::endl;
    std::cout << resp2.build() << std::endl;
    
    // Test redirect
    HttpResponse resp3 = HttpResponse::redirect("/new-location");
    std::cout << "Redirect Response:" << std::endl;
    std::cout << resp3.build() << std::endl;
}

void testStaticFileHandler() {
    std::cout << "=== Testing Static File Handler ===" << std::endl;
    
    StaticFileHandler handler("./www", true, "index.html");
    
    // Test GET request for file
    const char* request_data = 
        "GET /test.html HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "\r\n";
    
    HttpRequest req;
    req.parse(request_data, strlen(request_data));
    
    HttpResponse response = handler.handleRequest(req);
    std::cout << "Static file response (status " << response.getStatusCode() << "):" << std::endl;
    std::cout << response.build().substr(0, 200) << "..." << std::endl;
    std::cout << std::endl;
}

void testUploadHandler() {
    std::cout << "=== Testing Upload Handler ===" << std::endl;
    
    UploadHandler handler("./uploads", 10485760); // 10MB max
    
    // Simulate multipart form data
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string body = 
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "This is test file content\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";
    
    std::ostringstream request_stream;
    request_stream << "POST /upload HTTP/1.1\r\n"
                   << "Host: localhost:8080\r\n"
                   << "Content-Type: multipart/form-data; boundary=" << boundary << "\r\n"
                   << "Content-Length: " << body.length() << "\r\n"
                   << "\r\n"
                   << body;
    
    std::string request_str = request_stream.str();
    HttpRequest req;
    req.parse(request_str.c_str(), request_str.length());
    
    HttpResponse response = handler.handleUpload(req);
    std::cout << "Upload response (status " << response.getStatusCode() << "):" << std::endl;
    std::cout << response.build() << std::endl;
}

int main() {
    std::cout << "HTTP Components Test Program" << std::endl;
    std::cout << "=============================" << std::endl << std::endl;
    
    testHttpRequestParser();
    testHttpResponse();
    
    std::cout << "\nNote: File-based tests (static files and uploads) require" << std::endl;
    std::cout << "the ./www and ./uploads directories to exist." << std::endl;
    std::cout << "Create them and add test files to see full functionality." << std::endl;
    
    return 0;
}
