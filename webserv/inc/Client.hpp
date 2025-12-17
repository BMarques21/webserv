#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <ctime>

class Client {
private:
	int _fd;
	std::string _buffer;
	time_t _last_activity;
	bool _request_complete;
	size_t _content_length;
	size_t _body_received;
	bool _headers_parsed;

public:
	Client();
	Client(int fd);
	~Client();

	// Getters
	int getFd() const;
	std::string& getBuffer();
	const std::string& getBuffer() const;
	time_t getLastActivity() const;
	bool isRequestComplete() const;
	size_t getContentLength() const;
	size_t getBodyReceived() const;
	bool areHeadersParsed() const;

	// Setters
	void setRequestComplete(bool complete);
	void setContentLength(size_t length);
	void setBodyReceived(size_t received);
	void setHeadersParsed(bool parsed);
	void updateActivity();

	// Buffer management
	void addToBuffer(const std::string& data);
	void clearBuffer();
};

#endif // CLIENT_HPP

