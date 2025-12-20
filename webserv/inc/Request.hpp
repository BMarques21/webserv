#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>

class Request {
private:
	std::string _method;
	std::string _uri;
	std::string _version;
	std::map<std::string, std::string> _headers;
	std::string _body;
	bool _valid;
	std::string _error_message;

public:
	Request();
	~Request();

	// Parsing
	bool parse(const std::string& raw_request);

	// Getters
	const std::string& getMethod() const;
	const std::string& getUri() const;
	const std::string& getVersion() const;
	const std::map<std::string, std::string>& getHeaders() const;
	const std::string& getBody() const;
	bool isValid() const;
	const std::string& getErrorMessage() const;

	// Header lookup
	std::string getHeader(const std::string& key) const;
	bool hasHeader(const std::string& key) const;

private:
	void _parseRequestLine(const std::string& line);
	void _parseHeaders(const std::string& header_section);
	std::string _toLower(const std::string& str) const;
	std::string _trim(const std::string& str) const;
};

#endif // REQUEST_HPP

