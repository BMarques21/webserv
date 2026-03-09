#include "Client.hpp"

Client::Client() : _fd(-1), _last_activity(time(NULL)) {}

Client::Client(int fd) : _fd(fd), _last_activity(time(NULL)) {}

Client::~Client() {}

// Getters
int Client::getFd() const {
	return _fd;
}

HttpRequest& Client::getRequest() {
	return _request;
}

const HttpRequest& Client::getRequest() const {
	return _request;
}

time_t Client::getLastActivity() const {
	return _last_activity;
}

// Activity tracking
void Client::updateActivity() {
	_last_activity = time(NULL);
}

// Request management
void Client::resetRequest() {
	_request = HttpRequest();
}
