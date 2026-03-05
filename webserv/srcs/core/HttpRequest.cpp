#include "HttpRequest.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

static std::string toLower(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower;
}

HttpRequest::HttpRequest()
    : method(UNKNOWN), state(REQUEST_LINE), bytes_parsed(0),
      content_length(0), error_code(0) {
}

void HttpRequest::reset() {
    method = UNKNOWN;
    uri.clear();
    query_string.clear();
    http_version.clear();
    headers.clear();
    body.clear();
    state = REQUEST_LINE;
    raw_data.clear();
    bytes_parsed = 0;
    content_length = 0;
    boundary.clear();
    error_code = 0;
}

HttpMethod HttpRequest::stringToMethod(const std::string& method_str) {
    if (method_str == "GET") return GET;
    if (method_str == "POST") return POST;
    if (method_str == "DELETE") return DELETE;
    if (method_str == "PUT") return PUT;
    if (method_str == "HEAD") return HEAD;
    return UNKNOWN;
}

std::string HttpRequest::getMethodString() const {
    switch (method) {
        case GET: return "GET";
        case POST: return "POST";
        case DELETE: return "DELETE";
        case PUT: return "PUT";
        case HEAD: return "HEAD";
        default: return "UNKNOWN";
    }
}

void HttpRequest::parseRequestLine(const std::string& line) {
    std::istringstream iss(line);
    std::string method_str;

    iss >> method_str >> uri >> http_version;

    method = stringToMethod(method_str);

    if (method == UNKNOWN) {
        error_code = 405; // Method Not Allowed
        state = ERROR;
        return;
    }

    if (uri.empty() || http_version.empty()) {
        error_code = 400; // Bad Request
        state = ERROR;
        return;
    }

    // Check HTTP version
    if (http_version != "HTTP/1.1" && http_version != "HTTP/1.0") {
        error_code = 505; // HTTP Version Not Supported
        state = ERROR;
        return;
    }

    parseQueryString();
    state = HEADERS;
}

void HttpRequest::parseQueryString() {
    size_t pos = uri.find('?');
    if (pos != std::string::npos) {
        query_string = uri.substr(pos + 1);
        uri = uri.substr(0, pos);
    }
}

void HttpRequest::parseHeader(const std::string& line) {
    size_t pos = line.find(':');
    if (pos == std::string::npos) {
        error_code = 400; // Bad Request
        state = ERROR;
        return;
    }

    std::string key = trim(line.substr(0, pos));
    std::string value = trim(line.substr(pos + 1));

    // Convert header names to lowercase for case-insensitive lookup
    headers[toLower(key)] = value;
}

std::string HttpRequest::getHeader(const std::string& key) const {
    std::string lower_key = toLower(key);
    std::map<std::string, std::string>::const_iterator it = headers.find(lower_key);
    if (it != headers.end())
        return it->second;
    return "";
}

bool HttpRequest::isChunked() const {
    std::string transfer_encoding = getHeader("Transfer-Encoding");
    return toLower(transfer_encoding).find("chunked") != std::string::npos;
}

bool HttpRequest::parse(const char* data, size_t len) {
    if (state == COMPLETE || state == ERROR)
        return state == COMPLETE;

    raw_data.append(data, len);

    while (state != COMPLETE && state != ERROR) {
        if (state == REQUEST_LINE || state == HEADERS) {
            // Support both \r\n and bare \n line endings
            size_t line_end = raw_data.find('\n', bytes_parsed);
            if (line_end == std::string::npos) {
                // Check for request line/header size limits
                if (raw_data.size() > 8192) { // 8KB limit for headers
                    error_code = 431; // Request Header Fields Too Large
                    state = ERROR;
                }
                return false; // Need more data
            }

            // Strip optional \r before \n
            size_t line_len = line_end - bytes_parsed;
            if (line_len > 0 && raw_data[line_end - 1] == '\r')
                line_len--;
            std::string line = raw_data.substr(bytes_parsed, line_len);
            bytes_parsed = line_end + 1; // Skip past \n
            if (state == REQUEST_LINE) {
                parseRequestLine(line);
            } else if (state == HEADERS) {
                if (line.empty()) {
                    // Empty line marks end of headers
                    std::string content_len_str = getHeader("Content-Length");
                    bool has_content_length = !content_len_str.empty();
                    if (has_content_length) {
                        std::istringstream iss(content_len_str);
                        iss >> content_length;
                    }

                    // Extract boundary for multipart/form-data
                    std::string content_type = getHeader("Content-Type");
                    if (content_type.find("multipart/form-data") != std::string::npos) {
                        size_t boundary_pos = content_type.find("boundary=");
                        if (boundary_pos != std::string::npos) {
                            boundary = content_type.substr(boundary_pos + 9);
                            // Remove quotes if present (C++98 compatible)
                            if (!boundary.empty() && boundary[0] == '"' &&
                                boundary[boundary.length() - 1] == '"') {
                                boundary = boundary.substr(1, boundary.length() - 2);
                            }
                        }
                    }

                    // If Content-Length is present, it takes priority over chunked
                    // If no Content-Length and chunked, use chunked
                    // If neither, body is empty
                    if (has_content_length) {
                        if (content_length > 0)
                            state = BODY;
                        else
                            state = COMPLETE;
                    } else if (isChunked()) {
                        state = BODY;
                    } else {
                        state = COMPLETE;
                    }
                } else {
                    parseHeader(line);
                }
            }
        } else if (state == BODY) {
            if (isChunked()) {
                // Parse chunked transfer encoding
                while (true) {
                    // Find the chunk size line (support \r\n and bare \n)
                    size_t line_end = raw_data.find('\n', bytes_parsed);
                    if (line_end == std::string::npos)
                        return false; // Need more data

                    size_t sz_len = line_end - bytes_parsed;
                    if (sz_len > 0 && raw_data[line_end - 1] == '\r')
                        sz_len--;
                    std::string size_str = raw_data.substr(bytes_parsed, sz_len);
                    // Parse hex chunk size
                    size_t chunk_size = 0;
                    for (size_t i = 0; i < size_str.length(); ++i) {
                        char c = size_str[i];
                        if (c >= '0' && c <= '9')
                            chunk_size = chunk_size * 16 + (c - '0');
                        else if (c >= 'a' && c <= 'f')
                            chunk_size = chunk_size * 16 + (c - 'a' + 10);
                        else if (c >= 'A' && c <= 'F')
                            chunk_size = chunk_size * 16 + (c - 'A' + 10);
                        else
                            break; // chunk extensions, ignore
                    }

                    bytes_parsed = line_end + 1; // Skip past \n

                    if (chunk_size == 0) {
                        // Last chunk, skip optional trailers and final line ending
                        size_t final_lf = raw_data.find('\n', bytes_parsed);
                        if (final_lf != std::string::npos)
                            bytes_parsed = final_lf + 1;
                        content_length = body.size();
                        state = COMPLETE;
                        break;
                    }

                    // Need chunk_size bytes + trailing line ending
                    // Find the \n after chunk data
                    size_t chunk_data_end = bytes_parsed + chunk_size;
                    if (chunk_data_end >= raw_data.size())
                        return false; // Need more data
                    // Find next \n after chunk data
                    size_t next_lf = raw_data.find('\n', chunk_data_end);
                    if (next_lf == std::string::npos)
                        return false; // Need more data

                    body.append(raw_data, bytes_parsed, chunk_size);
                    bytes_parsed = next_lf + 1; // Skip chunk data + line ending
                }
            } else {
                // Read based on Content-Length
                size_t available = raw_data.size() - bytes_parsed;
                if (available >= content_length) {
                    body = raw_data.substr(bytes_parsed, content_length);
                    bytes_parsed += content_length;
                    state = COMPLETE;
                } else {
                    // Need more data
                    return false;
                }
            }
        }
    }

    return state == COMPLETE;
}

