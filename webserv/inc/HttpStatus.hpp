#ifndef HTTPSTATUS_HPP
#define HTTPSTATUS_HPP

#include <string>

class HttpStatus {
public:
	// Status codes
	static const int OK = 200;
	static const int CREATED = 201;
	static const int NO_CONTENT = 204;
	static const int MOVED_PERMANENTLY = 301;
	static const int FOUND = 302;
	static const int BAD_REQUEST = 400;
	static const int FORBIDDEN = 403;
	static const int NOT_FOUND = 404;
	static const int METHOD_NOT_ALLOWED = 405;
	static const int REQUEST_TIMEOUT = 408;
	static const int PAYLOAD_TOO_LARGE = 413;
	static const int INTERNAL_SERVER_ERROR = 500;
	static const int NOT_IMPLEMENTED = 501;
	static const int HTTP_VERSION_NOT_SUPPORTED = 505;

	// Get status message
	static std::string getMessage(int code);
};

#endif // HTTPSTATUS_HPP

