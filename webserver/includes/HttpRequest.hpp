#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include <vector>

enum HttpMethod {
    GET,
    POST,
    DELETE,
    PUT,
    HEAD,
    UNKNOWN
};

enum ParseState {
    REQUEST_LINE,
    HEADERS,
    BODY,
    COMPLETE,
    ERROR
};

class HttpRequest {
private:
    HttpMethod method;
    std::string uri;
    std::string query_string;
    std::string http_version;
    std::map<std::string, std::string> headers;
    std::string body;
    ParseState state;
    std::string raw_data;
    size_t bytes_parsed;
    size_t content_length;
    std::string boundary; // For multipart/form-data
    int error_code;

    void parseRequestLine(const std::string& line);
    void parseHeader(const std::string& line);
    HttpMethod stringToMethod(const std::string& method_str);
    void parseQueryString();
    bool isChunked() const;

public:
    HttpRequest();
    
    // Main parsing method - returns true if request is complete
    bool parse(const char* data, size_t len);
    
    // Getters
    HttpMethod getMethod() const { return method; }
    std::string getMethodString() const;
    const std::string& getUri() const { return uri; }
    const std::string& getQueryString() const { return query_string; }
    const std::string& getHttpVersion() const { return http_version; }
    const std::map<std::string, std::string>& getHeaders() const { return headers; }
    std::string getHeader(const std::string& key) const;
    const std::string& getBody() const { return body; }
    ParseState getState() const { return state; }
    int getErrorCode() const { return error_code; }
    size_t getContentLength() const { return content_length; }
    const std::string& getBoundary() const { return boundary; }
    
    // Validation
    bool isValid() const { return state != ERROR; }
    bool isComplete() const { return state == COMPLETE; }
    
    // Reset for reuse
    void reset();
};

#endif
