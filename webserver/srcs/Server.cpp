#include "Server.hpp"
#include "Client.hpp"
#include "Config.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "StaticFileHandler.hpp"
#include "UploadHandler.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

Server::Server(const std::string& config_file) : _config(NULL), _server_fd(-1) {
	_config = new Config(config_file);
	if (!_config->parse()) {
		delete _config;
		throw std::runtime_error("Failed to parse configuration file");
	}
	_setupSocket();
}

Server::~Server() {
	// Close all client connections
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		delete it->second;
		close(it->first);
	}

	// Close server socket
	if (_server_fd != -1)
		close(_server_fd);

	delete _config;
}

void Server::run() {
	const ServerConfig& server_config = _config->getServerConfig(0);
	std::cout << "Server running on " << server_config.host << ":" << server_config.port << std::endl;
	std::cout << "Waiting for connections..." << std::endl;

	extern volatile sig_atomic_t g_shutdown;

	while (!g_shutdown) {
		int poll_count = poll(&_poll_fds[0], _poll_fds.size(), 1000); // 1 second timeout

		if (poll_count < 0) {
			if (errno == EINTR) continue;
			throw std::runtime_error("Poll failed");
		}

		// Check for timeout cleanup
		_cleanupTimedOutClients();

		for (size_t i = 0; i < _poll_fds.size(); ) {
			if (_poll_fds[i].revents == 0) {
				i++;
				continue;
			}

			int current_fd = _poll_fds[i].fd;

			// Check for errors
			if (_poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
				if (current_fd == _server_fd) {
					std::cerr << "Error on server socket" << std::endl;
					i++;
					continue;
				}
				_removeClient(current_fd);
				continue;
			}

			// Handle POLLIN (incoming data)
			if (_poll_fds[i].revents & POLLIN) {
				if (current_fd == _server_fd) {
					_acceptNewClient();
				} else {
					_handleClientData(current_fd);
					if (_clients.find(current_fd) == _clients.end()) {
						continue;
					}
				}
			}

			// Check if i is still valid
			if (i >= _poll_fds.size() || _poll_fds[i].fd != current_fd) {
				continue;
			}

			// Handle POLLOUT (ready to write)
			if (_poll_fds[i].revents & POLLOUT) {
				if (_output_buffers.find(current_fd) != _output_buffers.end() &&
				    !_output_buffers[current_fd].empty()) {
					_flushClientBuffer(current_fd);
				}
			}

			i++;
		}
	}

	std::cout << "Closing all connections..." << std::endl;
}

//
/* Socket setup */
//

void Server::_setupSocket() {
	const ServerConfig& config = _config->getServerConfig(0);

	// Create server socket
	_server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_server_fd < 0) {
		throw std::runtime_error("Failed to create socket");
	}

	// Allow port reuse
	int opt = 1;
	if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		close(_server_fd);
		throw std::runtime_error("Failed to set socket options");
	}

	// Bind to port
	struct sockaddr_in address;
	address.sin_family = AF_INET;

	// Convert host string to network address
	if (inet_pton(AF_INET, config.host.c_str(), &address.sin_addr) <= 0) {
		address.sin_addr.s_addr = INADDR_ANY;
	}

	address.sin_port = htons(config.port);

	if (bind(_server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		close(_server_fd);
		std::ostringstream oss;
		oss << "Failed to bind socket to port " << config.port;
		throw std::runtime_error(oss.str());
	}

	// Listen for connections
	if (listen(_server_fd, LISTEN_CONN) < 0) {
		close(_server_fd);
		throw std::runtime_error("Failed to listen on server socket");
	}

	_setNonBlocking(_server_fd);

	// Add server socket to poll array
	struct pollfd server_pollfd;
	server_pollfd.fd = _server_fd;
	server_pollfd.events = POLLIN;
	server_pollfd.revents = 0;
	_poll_fds.push_back(server_pollfd);
}

void Server::_acceptNewClient() {
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	int client_fd = accept(_server_fd, (struct sockaddr*)&client_addr, &client_len);
	if (client_fd < 0) {
		std::cerr << "Failed to accept client connection" << std::endl;
		return;
	}

	_setNonBlocking(client_fd);

	// Add to poll array
	struct pollfd client_pollfd;
	client_pollfd.fd = client_fd;
	client_pollfd.events = POLLIN;
	client_pollfd.revents = 0;
	_poll_fds.push_back(client_pollfd);

	// Create client instance
	_clients[client_fd] = new Client(client_fd);
	std::cout << "New client connected: fd=" << client_fd << std::endl;
}

void Server::_handleClientData(int client_fd) {
	char buffer[BUFFER_SIZE];
	int bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);

	if (bytes_read <= 0) {
		if (bytes_read == 0) {
			std::cout << "Client disconnected: fd=" << client_fd << std::endl;
		} else {
			std::cerr << "Error reading from client: fd=" << client_fd << std::endl;
		}
		_removeClient(client_fd);
		return;
	}

	Client* client = _clients[client_fd];
	client->updateActivity();

	// Parse chunk incrementally using your HttpRequest parser
	bool complete = client->getRequest().parse(buffer, bytes_read);

	if (complete) {
		_processClientRequest(client_fd);
	}
}

void Server::_setNonBlocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
		flags = 0;
	}
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

//
/* Request processing */
//

void Server::_processClientRequest(int client_fd) {
	Client* client = _clients[client_fd];
	HttpRequest& request = client->getRequest();

	_handleRequest(client_fd, request);
	client->resetRequest();
}

void Server::_handleRequest(int client_fd, HttpRequest& request) {
	std::cout << "Request: " << request.getMethodString() << " " << request.getUri() << std::endl;

	HttpResponse response = _buildResponse(request);
	_sendToClient(client_fd, response.build());
}

HttpResponse Server::_buildResponse(const HttpRequest& request) {
	const ServerConfig& server_config = _config->getServerConfig(0);
	const LocationConfig* location = _config->findLocation(request.getUri(), server_config);

	if (!location) {
		return HttpResponse::notFound("Location not configured");
	}

	// Check if method is allowed
	std::string method_str = request.getMethodString();
	bool method_allowed = false;
	for (size_t i = 0; i < location->methods.size(); ++i) {
		if (location->methods[i] == method_str) {
			method_allowed = true;
			break;
		}
	}

	if (!method_allowed) {
		return HttpResponse::methodNotAllowed("Method not allowed for this location");
	}

	HttpMethod method = request.getMethod();

	// GET or DELETE -> Use StaticFileHandler
	if (method == GET || method == DELETE) {
		StaticFileHandler handler(location->root);
		return handler.handleRequest(request);
	}

	// HEAD -> Same as GET but no body
	else if (method == HEAD) {
		StaticFileHandler handler(location->root);
		HttpResponse response = handler.handleRequest(request);
		// HEAD is like GET but returns only headers, no body
		// We still need to return Content-Length header
		return response;
	}

	// POST -> Use UploadHandler
	else if (method == POST) {
		std::string upload_path = location->upload_path.empty() ? "./uploads" : location->upload_path;
		UploadHandler uploader(upload_path, server_config.max_body_size);
		return uploader.handleUpload(request);
	}

	// PUT -> Create/Update resource (for testing, return success message)
	else if (method == PUT) {
		// PUT is for creating/updating resources
		// For HTTP testing, return a success response
		return HttpResponse::ok("<html><body><h1>201 Created</h1><p>Resource created/updated via PUT</p></body></html>", "text/html");
	}

	else {
		return HttpResponse::badRequest("Method not implemented");
	}
}

//
/* Output handling */
//

void Server::_sendToClient(int client_fd, const std::string& data) {
	_output_buffers[client_fd] += data;

	// Update poll events to include POLLOUT
	for (size_t i = 0; i < _poll_fds.size(); i++) {
		if (_poll_fds[i].fd == client_fd) {
			_poll_fds[i].events |= POLLOUT;
			break;
		}
	}
}

void Server::_flushClientBuffer(int client_fd) {
	std::string& buffer = _output_buffers[client_fd];
	if (buffer.empty()) {
		return;
	}

	ssize_t sent = send(client_fd, buffer.c_str(), buffer.length(), 0);
	if (sent > 0) {
		buffer.erase(0, static_cast<size_t>(sent));
	} else if (sent <= 0) {
		_removeClient(client_fd);
		return;
	}

	// If buffer is empty, remove POLLOUT from events
	if (buffer.empty()) {
		for (size_t i = 0; i < _poll_fds.size(); i++) {
			if (_poll_fds[i].fd == client_fd) {
				_poll_fds[i].events = POLLIN;
				break;
			}
		}
	}
}

//
/* Client management */
//

void Server::_removeClient(int client_fd) {
	// Remove from poll_fds
	for (std::vector<struct pollfd>::iterator it = _poll_fds.begin(); it != _poll_fds.end(); ++it) {
		if (it->fd == client_fd) {
			_poll_fds.erase(it);
			break;
		}
	}

	// Delete client and remove from map
	if (_clients.find(client_fd) != _clients.end()) {
		delete _clients[client_fd];
		_clients.erase(client_fd);
	}

	// Remove output buffer
	_output_buffers.erase(client_fd);

	close(client_fd);
}

void Server::_cleanupTimedOutClients() {
	const time_t timeout = 60; // 60 seconds timeout
	time_t now = time(NULL);

	std::vector<int> clients_to_remove;

	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (now - it->second->getLastActivity() > timeout) {
			clients_to_remove.push_back(it->first);
		}
	}

	for (size_t i = 0; i < clients_to_remove.size(); ++i) {
		std::cout << "Client timeout: fd=" << clients_to_remove[i] << std::endl;
		_removeClient(clients_to_remove[i]);
	}
}

//
/* Helper methods (kept for backward compatibility) */
//

std::string Server::_readFile(const std::string& path) {
	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file.is_open()) {
		return "";
	}

	std::ostringstream contents;
	contents << file.rdbuf();
	return contents.str();
}

std::string Server::_getContentType(const std::string& path) {
	size_t dot_pos = path.find_last_of('.');
	if (dot_pos == std::string::npos) {
		return "application/octet-stream";
	}

	std::string ext = path.substr(dot_pos);
	if (ext == ".html" || ext == ".htm") return "text/html";
	if (ext == ".css") return "text/css";
	if (ext == ".js") return "application/javascript";
	if (ext == ".json") return "application/json";
	if (ext == ".png") return "image/png";
	if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
	if (ext == ".gif") return "image/gif";
	if (ext == ".svg") return "image/svg+xml";
	if (ext == ".txt") return "text/plain";
	if (ext == ".pdf") return "application/pdf";

	return "application/octet-stream";
}

bool Server::_fileExists(const std::string& path) {
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
}

void Server::_handleCgiRequest(int client_fd, const HttpRequest& request) {
	// TODO: Implement CGI handling
	(void)client_fd;
	(void)request;
}
