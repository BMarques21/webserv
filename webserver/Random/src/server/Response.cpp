#include "Response.hpp"
#include <sstream>

Response::Response() : _status_code(200) {
	_status_message = _getDefaultStatusMessage(200);
}

Response::Response(int status_code) : _status_code(status_code) {
	_status_message = _getDefaultStatusMessage(status_code);
}

Response::~Response() {}

void Response::setStatus(int code) {
	_status_code = code;
	_status_message = _getDefaultStatusMessage(code);
}

void Response::setStatusMessage(const std::string& message) {
	_status_message = message;
}

void Response::setHeader(const std::string& key, const std::string& value) {
	_headers[key] = value;
}

void Response::setBody(const std::string& body) {
	_body = body;
}

int Response::getStatusCode() const {
	return _status_code;
}

const std::string& Response::getStatusMessage() const {
	return _status_message;
}

const std::string& Response::getBody() const {
	return _body;
}

std::string Response::build() const {
	std::ostringstream response;

	// Status line
	response << "HTTP/1.1 " << _status_code << " " << _status_message << "\r\n";

	// Headers
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
	     it != _headers.end(); ++it) {
		response << it->first << ": " << it->second << "\r\n";
	}

	// Ensure Content-Length is set
	if (_headers.find("Content-Length") == _headers.end()) {
		std::ostringstream len;
		len << _body.length();
		response << "Content-Length: " << len.str() << "\r\n";
	}

	// Empty line before body
	response << "\r\n";

	// Body
	response << _body;

	return response.str();
}

std::string Response::_getDefaultStatusMessage(int code) const {
	switch (code) {
		case 200: return "OK";
		case 201: return "Created";
		case 204: return "No Content";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 413: return "Payload Too Large";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 505: return "HTTP Version Not Supported";
		default: return "Unknown";
	}
}

