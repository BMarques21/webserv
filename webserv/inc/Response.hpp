#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>

class Response {
private:
	int _status_code;
	std::string _status_message;
	std::map<std::string, std::string> _headers;
	std::string _body;

public:
	Response();
	Response(int status_code);
	~Response();

	// Setters
	void setStatus(int code);
	void setStatusMessage(const std::string& message);
	void setHeader(const std::string& key, const std::string& value);
	void setBody(const std::string& body);

	// Getters
	int getStatusCode() const;
	const std::string& getStatusMessage() const;
	const std::string& getBody() const;

	// Build HTTP response
	std::string build() const;

private:
	std::string _getDefaultStatusMessage(int code) const;
};

#endif // RESPONSE_HPP

