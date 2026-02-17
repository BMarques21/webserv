#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>

class Request;
class Response;
struct LocationConfig;

class CgiHandler {
private:
	std::string _cgi_path;
	std::string _script_path;
	const Request& _request;

public:
	CgiHandler(const std::string& cgi_path, const std::string& script_path,
	           const Request& request, const LocationConfig* location);
	~CgiHandler();

	// Execute CGI and return response
	bool execute(Response& response);

private:
	char** _buildEnv() const;
	void _freeEnv(char** env) const;
	std::string _getCgiExtension() const;
};

#endif // CGIHANDLER_HPP

