#include "HttpResponse.hpp"
#include <sstream>
#include <fstream>

std::string HttpResponse::loadErrorPage(const std::string& error_code) {
    std::string file_path = "www/errors/" + error_code + ".html";
    std::ifstream file(file_path.c_str());
    
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        return buffer.str();
    }
    
    // Fallback to generic error response if file not found
    return "<html><body><h1>" + error_code + " Error</h1></body></html>";
}

HttpResponse::HttpResponse() 
    : status_code(200), status_message("OK"), headers_sent(false) {
    setHeader("Server", "WebServ/1.0");
}

HttpResponse::HttpResponse(int code) 
    : status_code(code), headers_sent(false) {
    status_message = getStatusMessage(code);
    setHeader("Server", "WebServ/1.0");
}

std::string HttpResponse::getStatusMessage(int code) const {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 431: return "Request Header Fields Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 505: return "HTTP Version Not Supported";
        default: return "Unknown";
    }
}

void HttpResponse::setStatusCode(int code) {
    status_code = code;
    status_message = getStatusMessage(code);
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    headers[key] = value;
}

void HttpResponse::setBody(const std::string& content) {
    body = content;
    std::ostringstream oss;
    oss << content.size();
    setHeader("Content-Length", oss.str());
}

void HttpResponse::setContentType(const std::string& mime_type) {
    setHeader("Content-Type", mime_type);
}

std::string HttpResponse::build() {
    std::ostringstream response;
    
    // Status line
    response << "HTTP/1.1 " << status_code << " " << status_message << "\r\n";
    
    // Headers
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        response << it->first << ": " << it->second << "\r\n";
    }
    
    // Empty line separating headers from body
    response << "\r\n";
    
    // Body
    response << body;
    
    return response.str();
}

// Static helper methods
HttpResponse HttpResponse::ok(const std::string& content, const std::string& content_type) {
    HttpResponse response(200);
    response.setContentType(content_type);
    response.setBody(content);
    return response;
}

HttpResponse HttpResponse::created(const std::string& location) {
    HttpResponse response(201);
    if (!location.empty())
        response.setHeader("Location", location);
    response.setBody("<html><body><h1>201 Created</h1></body></html>");
    response.setContentType("text/html");
    return response;
}

HttpResponse HttpResponse::noContent() {
    HttpResponse response(204);
    return response;
}

HttpResponse HttpResponse::redirect(const std::string& location, int code) {
    HttpResponse response(code);
    response.setHeader("Location", location);
    std::string body = "<html><body><h1>Redirect</h1><p>Redirecting to <a href=\"" + 
                       location + "\">" + location + "</a></p></body></html>";
    response.setBody(body);
    response.setContentType("text/html");
    return response;
}

HttpResponse HttpResponse::badRequest(const std::string& message) {
    HttpResponse response(400);
    std::string body = "<html><body><h1>400 Bad Request</h1><p>" + message + "</p></body></html>";
    response.setBody(body);
    response.setContentType("text/html");
    return response;
}

HttpResponse HttpResponse::notFound(const std::string& message) {
    HttpResponse response(404);
    std::string body = loadErrorPage("404");
    if (body == "<html><body><h1>404 Error</h1></body></html>") {
        body = "<html><body><h1>404 Not Found</h1><p>" + message + "</p></body></html>";
    }
    response.setBody(body);
    response.setContentType("text/html");
    return response;
}

HttpResponse HttpResponse::methodNotAllowed(const std::string& message) {
    HttpResponse response(405);
    std::string body = "<html><body><h1>405 Method Not Allowed</h1><p>" + message + "</p></body></html>";
    response.setBody(body);
    response.setContentType("text/html");
    return response;
}

HttpResponse HttpResponse::internalServerError(const std::string& message) {
    HttpResponse response(500);
    std::string body = loadErrorPage("500");
    if (body == "<html><body><h1>500 Error</h1></body></html>") {
        body = "<html><body><h1>500 Internal Server Error</h1><p>" + message + "</p></body></html>";
    }
    response.setBody(body);
    response.setContentType("text/html");
    return response;
}

HttpResponse HttpResponse::notImplemented(const std::string& message) {
    HttpResponse response(501);
    std::string body = "<html><body><h1>501 Not Implemented</h1><p>" + message + "</p></body></html>";
    response.setBody(body);
    response.setContentType("text/html");
    return response;
}

HttpResponse HttpResponse::payloadTooLarge(const std::string& message) {
    HttpResponse response(413);
    std::string body = "<html><body><h1>413 Payload Too Large</h1><p>" + message + "</p></body></html>";
    response.setBody(body);
    response.setContentType("text/html");
    return response;
}
