#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <sys/poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#define LISTEN_CONN 128
#define BUFFER_SIZE 8192

class Client;
class Config;
class HttpRequest;
class HttpResponse;
struct LocationConfig;

class Server {
private:
	Config* _config;
	int _server_fd;
	std::vector<struct pollfd> _poll_fds;
	std::map<int, Client*> _clients; // fd -> Client*
	std::map<int, std::string> _output_buffers; // Output buffers per client fd

public:
	Server(const std::string& config_file);
	~Server();

	void run(); // Main event loop

private:
	// Socket setup
	void _setupSocket();
	void _acceptNewClient();
	void _handleClientData(int client_fd);
	void _setNonBlocking(int fd);

	// Request processing
	void _processClientRequest(int client_fd);
	void _handleRequest(int client_fd, HttpRequest& request);
	HttpResponse _buildResponse(const HttpRequest& request);

	// CGI handling
	void _handleCgiRequest(int client_fd, const HttpRequest& request);
	HttpResponse _executeCgi(const std::string& script_path, const std::string& interpreter,
	                         const HttpRequest& request, const LocationConfig& location);

	// Output handling
	void _sendToClient(int client_fd, const std::string& data);
	void _flushClientBuffer(int client_fd);

	// Client management
	void _removeClient(int client_fd);
	void _cleanupTimedOutClients();

	// Helper methods
	std::string _readFile(const std::string& path);
	std::string _getContentType(const std::string& path);
	bool _fileExists(const std::string& path);
};

#endif // SERVER_HPP
