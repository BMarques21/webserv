#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
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
struct ServerConfig;

class Server {
private:
	Config* _config;
	std::vector<int> _server_fds;
	std::map<int, size_t> _server_fd_to_index;
	std::map<int, size_t> _client_server_index;
	std::vector<struct pollfd> _poll_fds;
	std::map<int, Client*> _clients;
	std::map<int, std::string> _output_buffers;

public:
	Server(const std::string& config_file);
	~Server();
	void run();

private:
	void _setupSockets();
	bool _isServerFd(int fd) const;
	void _acceptNewClient(int server_fd);
	void _handleClientData(int client_fd);
	void _setNonBlocking(int fd);

	void _processClientRequest(int client_fd);
	void _handleRequest(int client_fd, HttpRequest& request);
	HttpResponse _buildResponse(int client_fd, const HttpRequest& request);

	void _handleCgiRequest(int client_fd, const HttpRequest& request);
	HttpResponse _executeCgi(const std::string& script_path, const std::string& interpreter,
	                         const HttpRequest& request, const LocationConfig& location,
	                         const ServerConfig& server_config);

	void _sendToClient(int client_fd, const std::string& data);
	void _flushClientBuffer(int client_fd);

	void _removeClient(int client_fd);
	void _cleanupTimedOutClients();

	std::string _readFile(const std::string& path);
	std::string _getContentType(const std::string& path);
	bool _fileExists(const std::string& path);
};

#endif // SERVER_HPP
