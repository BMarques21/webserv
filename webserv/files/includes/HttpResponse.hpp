#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>

class HttpResponse {
private:
    int status_code;
    std::string status_message;
    std::map<std::string, std::string> headers;
    std::string body;
    bool headers_sent;
    
    std::string getStatusMessage(int code) const;
    static std::string loadErrorPage(const std::string& error_code);
    
public:
    HttpResponse();
    HttpResponse(int code);
    
    // Setters
    void setStatusCode(int code);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& content);
    void setContentType(const std::string& mime_type);
    
    // Getters
    int getStatusCode() const { return status_code; }
    const std::string& getBody() const { return body; }
    
    // Build the complete HTTP response
    std::string build();
    
    // Common response builders
    static HttpResponse ok(const std::string& content, const std::string& content_type = "text/html");
    static HttpResponse created(const std::string& location = "");
    static HttpResponse noContent();
    static HttpResponse redirect(const std::string& location, int code = 302);
    static HttpResponse badRequest(const std::string& message = "Bad Request");
    static HttpResponse notFound(const std::string& message = "Not Found");
    static HttpResponse methodNotAllowed(const std::string& message = "Method Not Allowed");
    static HttpResponse internalServerError(const std::string& message = "Internal Server Error");
    static HttpResponse notImplemented(const std::string& message = "Not Implemented");
    static HttpResponse payloadTooLarge(const std::string& message = "Payload Too Large");
};

#endif
