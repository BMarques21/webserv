#include "CgiHandler.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Config.hpp"

#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <vector>

CgiHandler::CgiHandler(const std::string& cgi_path, const std::string& script_path,
                       const Request& request, const LocationConfig* location)
	: _cgi_path(cgi_path), _script_path(script_path), _request(request) {
	(void)location; // Suppress unused parameter warning
}

CgiHandler::~CgiHandler() {}

bool CgiHandler::execute(Response& response) {
	int pipe_in[2];
	int pipe_out[2];

	if (pipe(pipe_in) < 0 || pipe(pipe_out) < 0) {
		response.setStatus(500);
		response.setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
		response.setHeader("Content-Type", "text/html");
		return false;
	}

	pid_t pid = fork();
	if (pid < 0) {
		close(pipe_in[0]);
		close(pipe_in[1]);
		close(pipe_out[0]);
		close(pipe_out[1]);
		response.setStatus(500);
		response.setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
		response.setHeader("Content-Type", "text/html");
		return false;
	}

	if (pid == 0) {
		// Child process
		close(pipe_in[1]);
		close(pipe_out[0]);

		dup2(pipe_in[0], STDIN_FILENO);
		dup2(pipe_out[1], STDOUT_FILENO);

		close(pipe_in[0]);
		close(pipe_out[1]);

		char** env = _buildEnv();
		char* argv[] = { const_cast<char*>(_cgi_path.c_str()),
		                 const_cast<char*>(_script_path.c_str()), NULL };

		execve(_cgi_path.c_str(), argv, env);

		// If execve fails
		_freeEnv(env);
		exit(1);
	}

	// Parent process
	close(pipe_in[0]);
	close(pipe_out[1]);

	// Write request body to CGI
	const std::string& body = _request.getBody();
	if (!body.empty()) {
		write(pipe_in[1], body.c_str(), body.length());
	}
	close(pipe_in[1]);

	// Read CGI output
	std::string cgi_output;
	char buffer[4096];
	ssize_t bytes_read;

	while ((bytes_read = read(pipe_out[0], buffer, sizeof(buffer) - 1)) > 0) {
		buffer[bytes_read] = '\0';
		cgi_output += buffer;
	}
	close(pipe_out[0]);

	// Wait for child process
	int status;
	waitpid(pid, &status, 0);

	if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
		// Parse CGI output (headers + body)
		size_t header_end = cgi_output.find("\r\n\r\n");
		if (header_end == std::string::npos) {
			header_end = cgi_output.find("\n\n");
			if (header_end != std::string::npos) {
				header_end += 1; // For \n\n
			}
		} else {
			header_end += 2; // For \r\n\r\n
		}

		if (header_end != std::string::npos) {
			std::string headers = cgi_output.substr(0, header_end);
			std::string body = cgi_output.substr(header_end + 2);

			// Parse CGI headers
			std::istringstream header_stream(headers);
			std::string line;
			while (std::getline(header_stream, line)) {
				if (line.empty() || line == "\r") continue;

				size_t colon = line.find(':');
				if (colon != std::string::npos) {
					std::string key = line.substr(0, colon);
					std::string value = line.substr(colon + 1);

					// Trim whitespace
					while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) {
						value = value.substr(1);
					}
					while (!value.empty() && (value[value.length()-1] == '\r' || value[value.length()-1] == '\n')) {
						value = value.substr(0, value.length() - 1);
					}

					response.setHeader(key, value);
				}
			}

			response.setStatus(200);
			response.setBody(body);
		} else {
			response.setStatus(200);
			response.setBody(cgi_output);
			response.setHeader("Content-Type", "text/html");
		}
		return true;
	}

	response.setStatus(500);
	response.setBody("<html><body><h1>500 CGI Error</h1></body></html>");
	response.setHeader("Content-Type", "text/html");
	return false;
}

char** CgiHandler::_buildEnv() const {
	std::vector<std::string> env_strings;

	// Required CGI environment variables
	env_strings.push_back("REQUEST_METHOD=" + _request.getMethod());
	env_strings.push_back("SCRIPT_FILENAME=" + _script_path);
	env_strings.push_back("QUERY_STRING="); // TODO: Parse query string from URI
	env_strings.push_back("CONTENT_TYPE=" + _request.getHeader("Content-Type"));

	std::ostringstream len;
	len << _request.getBody().length();
	env_strings.push_back("CONTENT_LENGTH=" + len.str());

	env_strings.push_back("SERVER_PROTOCOL=" + _request.getVersion());
	env_strings.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env_strings.push_back("SERVER_SOFTWARE=webserv/1.0");
	env_strings.push_back("REDIRECT_STATUS=200");

	// Allocate environment array
	char** env = new char*[env_strings.size() + 1];
	for (size_t i = 0; i < env_strings.size(); ++i) {
		env[i] = new char[env_strings[i].length() + 1];
		std::strcpy(env[i], env_strings[i].c_str());
	}
	env[env_strings.size()] = NULL;

	return env;
}

void CgiHandler::_freeEnv(char** env) const {
	for (size_t i = 0; env[i] != NULL; ++i) {
		delete[] env[i];
	}
	delete[] env;
}

std::string CgiHandler::_getCgiExtension() const {
	size_t dot_pos = _script_path.find_last_of('.');
	if (dot_pos != std::string::npos) {
		return _script_path.substr(dot_pos);
	}
	return "";
}

