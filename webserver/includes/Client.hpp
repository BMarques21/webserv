#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "HttpRequest.hpp"
#include <string>
#include <ctime>

class Client {
private:
	int _fd;
	HttpRequest _request;
	time_t _last_activity;

public:
	Client();
	Client(int fd);
	~Client();

	// Getters
	int getFd() const;
	HttpRequest& getRequest();
	const HttpRequest& getRequest() const;
	time_t getLastActivity() const;

	// Activity tracking
	void updateActivity();

	// Request management
	void resetRequest();
};

#endif // CLIENT_HPP
