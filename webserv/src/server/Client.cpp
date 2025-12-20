#include "Client.hpp"

Client::Client() : _fd(-1), _last_activity(time(NULL)), _request_complete(false),
                   _content_length(0), _body_received(0), _headers_parsed(false) {}

Client::Client(int fd) : _fd(fd), _last_activity(time(NULL)), _request_complete(false),
                         _content_length(0), _body_received(0), _headers_parsed(false) {}

Client::~Client() {}

// Getters
int Client::getFd() const {
	return _fd;
}

std::string& Client::getBuffer() {
	return _buffer;
}

const std::string& Client::getBuffer() const {
	return _buffer;
}

time_t Client::getLastActivity() const {
	return _last_activity;
}

bool Client::isRequestComplete() const {
	return _request_complete;
}

size_t Client::getContentLength() const {
	return _content_length;
}

size_t Client::getBodyReceived() const {
	return _body_received;
}

bool Client::areHeadersParsed() const {
	return _headers_parsed;
}

// Setters
void Client::setRequestComplete(bool complete) {
	_request_complete = complete;
}

void Client::setContentLength(size_t length) {
	_content_length = length;
}

void Client::setBodyReceived(size_t received) {
	_body_received = received;
}

void Client::setHeadersParsed(bool parsed) {
	_headers_parsed = parsed;
}

void Client::updateActivity() {
	_last_activity = time(NULL);
}

// Buffer management
void Client::addToBuffer(const std::string& data) {
	_buffer += data;
}

void Client::clearBuffer() {
	_buffer.clear();
	_request_complete = false;
	_content_length = 0;
	_body_received = 0;
	_headers_parsed = false;
}

