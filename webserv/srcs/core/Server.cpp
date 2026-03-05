// Server.cpp - Multi-socket support: one listening socket per server{} block,
// all multiplexed via a single poll() loop.

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
#include <cstdlib>
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <poll.h>

Server::Server(const std::string& config_file) : _config(NULL) {
	_config = new Config(config_file);
	if (!_config->parse()) {
		delete _config;
		throw std::runtime_error("Failed to parse configuration file");
	}
	_setupSockets();
}

Server::~Server() {
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		delete it->second;
		close(it->first);
	}
	for (size_t i = 0; i < _server_fds.size(); ++i) {
		if (_server_fds[i] != -1)
			close(_server_fds[i]);
	}
	delete _config;
}

void Server::run() {
	const std::vector<ServerConfig>& servers = _config->getServers();
	for (size_t i = 0; i < servers.size(); ++i) {
		std::cout << "Server running on " << servers[i].host << ":" << servers[i].port << std::endl;
	}
	std::cout << "Waiting for connections..." << std::endl;

	extern volatile sig_atomic_t g_shutdown;

	while (!g_shutdown) {
		int poll_count = poll(&_poll_fds[0], _poll_fds.size(), 1000);
		if (poll_count < 0) {
			if (errno == EINTR) continue;
			throw std::runtime_error("Poll failed");
		}
		_cleanupTimedOutClients();

		for (size_t i = 0; i < _poll_fds.size(); ) {
			if (_poll_fds[i].revents == 0) { i++; continue; }
			int current_fd = _poll_fds[i].fd;

			if (_poll_fds[i].revents & (POLLERR | POLLNVAL)) {
				if (_isServerFd(current_fd)) { i++; continue; }
				_removeClient(current_fd);
				continue;
			}
			if (_poll_fds[i].revents & POLLIN) {
				if (_isServerFd(current_fd)) {
					_acceptNewClient(current_fd);
				} else {
					_handleClientData(current_fd);
					if (_clients.find(current_fd) == _clients.end()) continue;
				}
			}
			if (i >= _poll_fds.size() || _poll_fds[i].fd != current_fd) continue;
			if (_poll_fds[i].revents & POLLOUT) {
				if (_output_buffers.find(current_fd) != _output_buffers.end() &&
				    !_output_buffers[current_fd].empty()) {
					_flushClientBuffer(current_fd);
					if (_clients.find(current_fd) == _clients.end()) continue;
				}
			}
			if (i >= _poll_fds.size() || _poll_fds[i].fd != current_fd) { continue; }
			if (_poll_fds[i].revents & POLLHUP) {
				if (!_isServerFd(current_fd)) {

					// Se ainda há dados para enviar,
					// deixar POLLOUT cuidar
					if (_output_buffers.find(current_fd) != _output_buffers.end() &&
						!_output_buffers[current_fd].empty()) {

						_poll_fds[i].events = POLLOUT;
						i++;
						continue;
						}

					_removeClient(current_fd);
					continue;
				}
			}
			i++;
		}
	}
	std::cout << "Closing all connections..." << std::endl;
}

void Server::_setupSockets() {
	const std::vector<ServerConfig>& servers = _config->getServers();
	for (size_t idx = 0; idx < servers.size(); ++idx) {
		const ServerConfig& config = servers[idx];
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0) {
			std::ostringstream oss;
			oss << "Failed to create socket for port " << config.port;
			throw std::runtime_error(oss.str());
		}
		int opt = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
			close(fd);
			throw std::runtime_error("Failed to set socket options");
		}
		struct sockaddr_in address;
		std::memset(&address, 0, sizeof(address));
		address.sin_family = AF_INET;
		if (inet_pton(AF_INET, config.host.c_str(), &address.sin_addr) <= 0)
			address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(config.port);
		if (bind(fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
			close(fd);
			std::ostringstream oss;
			oss << "Failed to bind to " << config.host << ":" << config.port
			    << " (" << strerror(errno) << ")";
			throw std::runtime_error(oss.str());
		}
		if (listen(fd, LISTEN_CONN) < 0) {
			close(fd);
			std::ostringstream oss;
			oss << "Failed to listen on " << config.host << ":" << config.port;
			throw std::runtime_error(oss.str());
		}
		_setNonBlocking(fd);
		_server_fds.push_back(fd);
		_server_fd_to_index[fd] = idx;
		struct pollfd pfd;
		pfd.fd = fd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		_poll_fds.push_back(pfd);
	}
}

bool Server::_isServerFd(int fd) const {
	return _server_fd_to_index.find(fd) != _server_fd_to_index.end();
}

void Server::_acceptNewClient(int server_fd) {
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
	if (client_fd < 0) { std::cerr << "Failed to accept" << std::endl; return; }
	_setNonBlocking(client_fd);
	struct pollfd cpfd;
	cpfd.fd = client_fd;
	cpfd.events = POLLIN;
	cpfd.revents = 0;
	_poll_fds.push_back(cpfd);
	_clients[client_fd] = new Client(client_fd);
	_client_server_index[client_fd] = _server_fd_to_index[server_fd];
	std::cout << "New client fd=" << client_fd << " on server block " << _client_server_index[client_fd] << std::endl;
}

void Server::_handleClientData(int client_fd) {
	char buffer[BUFFER_SIZE];
	int bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
	if (bytes_read <= 0) {
		if (bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
			return;
		// Client disconnected (recv == 0) or error
		// If there's pending output, keep connection open for POLLOUT to flush
		if (bytes_read == 0 && _output_buffers.find(client_fd) != _output_buffers.end()
		    && !_output_buffers[client_fd].empty()) {
			for (size_t i = 0; i < _poll_fds.size(); ++i) {
				if (_poll_fds[i].fd == client_fd) {
					_poll_fds[i].events = POLLOUT;
					break;
				}
			}
			return;
		}
		_removeClient(client_fd);
		return;
	}
	Client* client = _clients[client_fd];
	client->updateActivity();
	bool complete = client->getRequest().parse(buffer, bytes_read);
	if (complete) {
		_processClientRequest(client_fd);
	} else if (client->getRequest().getState() == ERROR) {
		int err = client->getRequest().getErrorCode();
		HttpResponse resp;
		if (err == 405)
			resp = HttpResponse::methodNotAllowed("Method not allowed");
		else if (err == 505)
			resp = HttpResponse::notImplemented("HTTP Version Not Supported");
		else if (err == 501)
			resp = HttpResponse::notImplemented("Not Implemented");
		else
			resp = HttpResponse::badRequest("Bad Request");
		_output_buffers[client_fd] += resp.build();
		for (size_t i = 0; i < _poll_fds.size(); ++i) {
			if (_poll_fds[i].fd == client_fd) {
				_poll_fds[i].events = POLLIN | POLLOUT;
				break;
			}
		}
		client->resetRequest();
	}
}

void Server::_setNonBlocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) flags = 0;
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::_processClientRequest(int client_fd) {
	Client* client = _clients[client_fd];
	HttpRequest& request = client->getRequest();
	_handleRequest(client_fd, request);
	client->resetRequest();
}

void Server::_handleRequest(int client_fd, HttpRequest& request) {
	HttpResponse response = _buildResponse(client_fd, request);
	std::string data = response.build();

	_output_buffers[client_fd] += data;

	for (size_t i = 0; i < _poll_fds.size(); ++i) {
		if (_poll_fds[i].fd == client_fd) {
			_poll_fds[i].events = POLLIN | POLLOUT;
			break;
		}
	}
}


HttpResponse Server::_buildResponse(int client_fd, const HttpRequest& request) {
	size_t server_idx = 0;
	if (_client_server_index.find(client_fd) != _client_server_index.end())
		server_idx = _client_server_index[client_fd];
	const ServerConfig& server_config = _config->getServerConfig(server_idx);
	const LocationConfig* location = _config->findLocation(request.getUri(), server_config);
	if (!location) return HttpResponse::notFound("Location not configured");

	std::string method_str = request.getMethodString();
	bool method_allowed = false;
	for (size_t i = 0; i < location->methods.size(); ++i) {
		if (location->methods[i] == method_str) { method_allowed = true; break; }
	}
	if (!method_allowed) return HttpResponse::methodNotAllowed("Method not allowed for this location");

	HttpMethod method = request.getMethod();

	if (!location->cgi_extensions.empty()) {
		std::string uri = request.getUri();
		size_t qpos = uri.find('?');
		std::string path_part = (qpos != std::string::npos) ? uri.substr(0, qpos) : uri;
		size_t dot_pos = path_part.find_last_of('.');
		if (dot_pos != std::string::npos) {
			std::string ext = path_part.substr(dot_pos);
			std::map<std::string, std::string>::const_iterator it = location->cgi_extensions.find(ext);
			if (it != location->cgi_extensions.end()) {
				std::string script_path = location->root + "/" + path_part;
				return _executeCgi(script_path, it->second, request, *location, server_config);
			}
		}
	}

	if (method == GET || method == DELETE) {
		StaticFileHandler handler(location->root, location->autoindex, location->index);
		return handler.handleRequest(request);
	} else if (method == HEAD) {
		StaticFileHandler handler(location->root, location->autoindex, location->index);
		return handler.handleRequest(request);
	} else if (method == POST) {
		std::string content_type = request.getHeader("Content-Type");
		bool is_multipart = content_type.find("multipart/form-data") != std::string::npos;

		// Only route to UploadHandler when the location has an upload_path
		// AND the request is a multipart file upload
		if (!location->upload_path.empty() && is_multipart) {
			UploadHandler uploader(location->upload_path, server_config.max_body_size);
			return uploader.handleUpload(request);
		}

		// Plain-text / JSON / form-urlencoded POST: acknowledge with 200 OK
		std::string body = request.getBody();
		std::ostringstream resp_body;
		resp_body << "<html><body>"
		          << "<h1>200 OK</h1>"
		          << "<p>POST received (" << body.size() << " bytes)</p>"
		          << "</body></html>";
		return HttpResponse::ok(resp_body.str(), "text/html");
	} else if (method == PUT) {
		return HttpResponse::ok("<html><body><h1>201 Created</h1><p>Resource created/updated via PUT</p></body></html>", "text/html");
	}
	return HttpResponse::badRequest("Method not implemented");
}

void Server::_sendToClient(int client_fd, const std::string& data) {
	_output_buffers[client_fd] += data;
	for (size_t i = 0; i < _poll_fds.size(); i++) {
		if (_poll_fds[i].fd == client_fd) { _poll_fds[i].events = POLLIN | POLLOUT; break; }
	}
}

void Server::_flushClientBuffer(int client_fd) {
	std::string& buffer = _output_buffers[client_fd];
	if (buffer.empty()) return;
	ssize_t sent = send(client_fd, buffer.c_str(), buffer.length(), MSG_NOSIGNAL);
	if (sent > 0) {
		buffer.erase(0, static_cast<size_t>(sent));
	} else if (sent < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		_removeClient(client_fd);
		return;
	}
	if (buffer.empty()) {
		// Check if this fd was kept alive only for flushing (events == POLLOUT)
		bool flush_only = false;
		for (size_t i = 0; i < _poll_fds.size(); i++) {
			if (_poll_fds[i].fd == client_fd) {
				if (_poll_fds[i].events == POLLOUT)
					flush_only = true;
				else
					_poll_fds[i].events = POLLIN;
				break;
			}
		}
		if (flush_only) {
			_removeClient(client_fd);
		}
	}
}

void Server::_removeClient(int client_fd) {
	for (std::vector<struct pollfd>::iterator it = _poll_fds.begin(); it != _poll_fds.end(); ++it) {
		if (it->fd == client_fd) { _poll_fds.erase(it); break; }
	}
	if (_clients.find(client_fd) != _clients.end()) {
		delete _clients[client_fd];
		_clients.erase(client_fd);
	}
	_output_buffers.erase(client_fd);
	_client_server_index.erase(client_fd);
	close(client_fd);
}

void Server::_cleanupTimedOutClients() {
	const time_t timeout = 60;
	time_t now = time(NULL);
	std::vector<int> to_remove;
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (now - it->second->getLastActivity() > timeout)
			to_remove.push_back(it->first);
	}
	for (size_t i = 0; i < to_remove.size(); ++i) {
		std::cout << "Client timeout: fd=" << to_remove[i] << std::endl;
		_removeClient(to_remove[i]);
	}
}

std::string Server::_readFile(const std::string& path) {
	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file.is_open()) return "";
	std::ostringstream contents;
	contents << file.rdbuf();
	return contents.str();
}

std::string Server::_getContentType(const std::string& path) {
	size_t dot_pos = path.find_last_of('.');
	if (dot_pos == std::string::npos) return "application/octet-stream";
	std::string ext = path.substr(dot_pos);
	if (ext == ".html" || ext == ".htm") return "text/html";
	if (ext == ".css") return "text/css";
	if (ext == ".js") return "application/javascript";
	if (ext == ".json") return "application/json";
	if (ext == ".xml") return "application/xml";
	if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
	if (ext == ".png") return "image/png";
	if (ext == ".gif") return "image/gif";
	if (ext == ".svg") return "image/svg+xml";
	if (ext == ".txt") return "text/plain";
	if (ext == ".pdf") return "application/pdf";
	if (ext == ".zip") return "application/zip";
	if (ext == ".mp3") return "audio/mpeg";
	if (ext == ".mp4") return "video/mp4";
	if (ext == ".woff") return "font/woff";
	if (ext == ".woff2") return "font/woff2";
	if (ext == ".ttf") return "font/ttf";
	return "application/octet-stream";
}

bool Server::_fileExists(const std::string& path) {
	struct stat st;
	return (stat(path.c_str(), &st) == 0);
}

void Server::_handleCgiRequest(int client_fd, const HttpRequest& request) {
	(void)client_fd;
	(void)request;
}

HttpResponse Server::_executeCgi(const std::string& script_path,
                                  const std::string& interpreter,
                                  const HttpRequest& request,
                                  const LocationConfig& location,
                                  const ServerConfig& server_config)
{
    (void)location;

    struct stat st;
    if (stat(script_path.c_str(), &st) != 0 || !S_ISREG(st.st_mode))
        return HttpResponse::notFound("CGI script not found: " + script_path);

    std::string body = request.getBody();

    std::vector<std::string> env_strings;
    env_strings.push_back("REQUEST_METHOD=" + request.getMethodString());
    env_strings.push_back("SCRIPT_FILENAME=" + script_path);
    env_strings.push_back("SCRIPT_NAME=" + request.getUri());
    env_strings.push_back("SERVER_PROTOCOL=HTTP/1.1");
    env_strings.push_back("SERVER_NAME=" + server_config.host);

    std::ostringstream port_ss;
    port_ss << server_config.port;
    env_strings.push_back("SERVER_PORT=" + port_ss.str());

    if (request.getMethod() == POST) {
        std::ostringstream len_ss;
        len_ss << body.size();
        env_strings.push_back("CONTENT_LENGTH=" + len_ss.str());

        std::string ct = request.getHeader("Content-Type");
        if (ct.empty())
            ct = "application/x-www-form-urlencoded";
        env_strings.push_back("CONTENT_TYPE=" + ct);
    }

    std::vector<char*> envp;
    for (size_t i = 0; i < env_strings.size(); ++i)
        envp.push_back(const_cast<char*>(env_strings[i].c_str()));
    envp.push_back(NULL);

    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(interpreter.c_str()));
    argv.push_back(const_cast<char*>(script_path.c_str()));
    argv.push_back(NULL);

    int stdin_pipe[2];
    int stdout_pipe[2];

    if (pipe(stdin_pipe) < 0 || pipe(stdout_pipe) < 0)
        return HttpResponse::internalServerError("CGI pipe failed");

    pid_t pid = fork();
    if (pid < 0)
        return HttpResponse::internalServerError("CGI fork failed");

    if (pid == 0) {
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);

        close(stdin_pipe[1]);
        close(stdout_pipe[0]);

        execve(interpreter.c_str(), &argv[0], &envp[0]);
        _exit(1);
    }

    close(stdin_pipe[0]);
    close(stdout_pipe[1]);

    if (!body.empty())
        write(stdin_pipe[1], body.c_str(), body.size());
    close(stdin_pipe[1]);

    // 🔥 TIMEOUT MANUAL SEM POLL
    int status;
    time_t start = time(NULL);
    const int timeout = 5; // 5 segundos

    while (true) {
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == pid)
            break;

        if (time(NULL) - start > timeout) {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            close(stdout_pipe[0]);
            return HttpResponse::internalServerError("CGI timed out");
        }
        usleep(10000);
    }

    std::string cgi_output;
    char buffer[4096];
    ssize_t n;

    while ((n = read(stdout_pipe[0], buffer, sizeof(buffer))) > 0)
        cgi_output.append(buffer, n);

    close(stdout_pipe[0]);

    if (cgi_output.empty())
        return HttpResponse::internalServerError("CGI produced no output");

    size_t sep = cgi_output.find("\r\n\r\n");
    if (sep == std::string::npos)
        sep = cgi_output.find("\n\n");

    std::string body_part = (sep != std::string::npos)
        ? cgi_output.substr(sep + 4)
        : cgi_output;

    return HttpResponse::ok(body_part, "text/html");
}
