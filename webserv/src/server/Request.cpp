#include "Request.hpp"
#include <sstream>
#include <algorithm>

Request::Request() : _valid(false) {}

Request::~Request() {}

bool Request::parse(const std::string& raw_request) {
	if (raw_request.empty()) {
		_valid = false;
		_error_message = "Empty request";
		return false;
	}

	// Find header/body separator
	size_t header_end = raw_request.find("\r\n\r\n");
	if (header_end == std::string::npos) {
		_valid = false;
		_error_message = "Incomplete request headers";
		return false;
	}

	std::string header_section = raw_request.substr(0, header_end);
	_body = raw_request.substr(header_end + 4);

	// Parse request line and headers
	size_t first_line_end = header_section.find("\r\n");
	if (first_line_end == std::string::npos) {
		_valid = false;
		_error_message = "Invalid request line";
		return false;
	}

	std::string request_line = header_section.substr(0, first_line_end);
	_parseRequestLine(request_line);

	if (!_valid) {
		return false;
	}

	std::string headers_str = header_section.substr(first_line_end + 2);
	_parseHeaders(headers_str);

	_valid = true;
	return true;
}

void Request::_parseRequestLine(const std::string& line) {
	std::istringstream iss(line);
	iss >> _method >> _uri >> _version;

	if (_method.empty() || _uri.empty() || _version.empty()) {
		_valid = false;
		_error_message = "Invalid request line format";
		return;
	}

	// Convert method to uppercase
	std::transform(_method.begin(), _method.end(), _method.begin(), ::toupper);

	// Validate HTTP version
	if (_version != "HTTP/1.1" && _version != "HTTP/1.0") {
		_valid = false;
		_error_message = "Unsupported HTTP version";
		return;
	}

	_valid = true;
}

void Request::_parseHeaders(const std::string& header_section) {
	std::istringstream stream(header_section);
	std::string line;

	while (std::getline(stream, line)) {
		if (line.empty() || line == "\r") {
			continue;
		}

		// Remove trailing \r if present
		if (!line.empty() && line[line.length() - 1] == '\r') {
			line = line.substr(0, line.length() - 1);
		}

		size_t colon_pos = line.find(':');
		if (colon_pos == std::string::npos) {
			continue;
		}

		std::string key = _trim(line.substr(0, colon_pos));
		std::string value = _trim(line.substr(colon_pos + 1));

		// Convert key to lowercase for case-insensitive lookup
		_headers[_toLower(key)] = value;
	}
}

std::string Request::_toLower(const std::string& str) const {
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}

std::string Request::_trim(const std::string& str) const {
	size_t start = 0;
	size_t end = str.length();

	while (start < end && (str[start] == ' ' || str[start] == '\t')) {
		start++;
	}

	while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t')) {
		end--;
	}

	return str.substr(start, end - start);
}

// Getters
const std::string& Request::getMethod() const { return _method; }
const std::string& Request::getUri() const { return _uri; }
const std::string& Request::getVersion() const { return _version; }
const std::map<std::string, std::string>& Request::getHeaders() const { return _headers; }
const std::string& Request::getBody() const { return _body; }
bool Request::isValid() const { return _valid; }
const std::string& Request::getErrorMessage() const { return _error_message; }

std::string Request::getHeader(const std::string& key) const {
	std::string lower_key = _toLower(key);
	std::map<std::string, std::string>::const_iterator it = _headers.find(lower_key);
	if (it != _headers.end()) {
		return it->second;
	}
	return "";
}

bool Request::hasHeader(const std::string& key) const {
	return _headers.find(_toLower(key)) != _headers.end();
}

