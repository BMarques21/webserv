// Example integration with poll()-based server
// This shows how to use the HTTP components in a non-blocking server

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "StaticFileHandler.hpp"
#include "UploadHandler.hpp"
#include <poll.h>
#include <map>
#include <vector>
#include <unistd.h>
#include <fcntl.h>

// Client connection state
struct ClientConnection {
    int socket_fd;
    HttpRequest request;
    std::string response_buffer;
    size_t bytes_sent;
    bool request_complete;
    bool response_ready;
    
    ClientConnection(int fd) 
        : socket_fd(fd), bytes_sent(0), 
          request_complete(false), response_ready(false) {
        // Set socket to non-blocking
        fcntl(fd, F_SETFL, O_NONBLOCK);
    }
};

class WebServer {
private:
    int listen_socket;
    std::map<int, ClientConnection*> clients;
    StaticFileHandler static_handler;
    UploadHandler upload_handler;
    
    void handleNewConnection() {
        int client_fd = accept(listen_socket, NULL, NULL);
        if (client_fd < 0)
            return;
        
        clients[client_fd] = new ClientConnection(client_fd);
    }
    
    void handleClientRead(int fd) {
        ClientConnection* client = clients[fd];
        char buffer[4096];
        
        ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            // Connection closed or error
            closeClient(fd);
            return;
        }
        
        // Parse request incrementally
        bool complete = client->request.parse(buffer, bytes);
        
        if (complete) {
            client->request_complete = true;
            processRequest(client);
        }
    }
    
    void processRequest(ClientConnection* client) {
        HttpResponse response;
        const HttpRequest& req = client->request;
        
        // Route the request
        if (req.getUri().find("/upload") == 0 && req.getMethod() == POST) {
            // Handle file upload
            response = upload_handler.handleUpload(req);
        } 
        else if (req.getMethod() == GET) {
            // Serve static files
            response = static_handler.handleRequest(req);
        } 
        else if (req.getMethod() == DELETE) {
            // Handle DELETE (you'd implement file deletion here)
            response = HttpResponse::noContent();
        } 
        else {
            response = HttpResponse::methodNotAllowed();
        }
        
        // Prepare response for sending
        client->response_buffer = response.build();
        client->response_ready = true;
        client->bytes_sent = 0;
    }
    
    void handleClientWrite(int fd) {
        ClientConnection* client = clients[fd];
        
        if (!client->response_ready)
            return;
        
        size_t remaining = client->response_buffer.length() - client->bytes_sent;
        const char* data = client->response_buffer.c_str() + client->bytes_sent;
        
        ssize_t sent = send(fd, data, remaining, 0);
        if (sent > 0) {
            client->bytes_sent += sent;
            
            // If entire response sent, close connection
            if (client->bytes_sent >= client->response_buffer.length()) {
                closeClient(fd);
            }
        } else if (sent < 0) {
            // Error or would block
            closeClient(fd);
        }
    }
    
    void closeClient(int fd) {
        close(fd);
        delete clients[fd];
        clients.erase(fd);
    }
    
public:
    WebServer(int port) 
        : static_handler("./www", true, "index.html"),
          upload_handler("./uploads", 10485760) {
        
        // Setup listen socket (simplified)
        listen_socket = socket(AF_INET, SOCK_STREAM, 0);
        // ... bind, listen, set non-blocking ...
    }
    
    void run() {
        std::vector<struct pollfd> poll_fds;
        
        while (true) {
            poll_fds.clear();
            
            // Add listen socket
            struct pollfd listen_pfd;
            listen_pfd.fd = listen_socket;
            listen_pfd.events = POLLIN;
            poll_fds.push_back(listen_pfd);
            
            // Add client sockets
            for (std::map<int, ClientConnection*>::iterator it = clients.begin();
                 it != clients.end(); ++it) {
                
                struct pollfd pfd;
                pfd.fd = it->first;
                pfd.events = 0;
                
                // Monitor for reading if request not complete
                if (!it->second->request_complete)
                    pfd.events |= POLLIN;
                
                // Monitor for writing if response ready
                if (it->second->response_ready)
                    pfd.events |= POLLOUT;
                
                poll_fds.push_back(pfd);
            }
            
            // Wait for events (timeout after 1 second)
            int ready = poll(&poll_fds[0], poll_fds.size(), 1000);
            
            if (ready <= 0)
                continue;
            
            // Process events
            for (size_t i = 0; i < poll_fds.size(); ++i) {
                if (poll_fds[i].revents == 0)
                    continue;
                
                int fd = poll_fds[i].fd;
                
                if (fd == listen_socket) {
                    // New connection
                    if (poll_fds[i].revents & POLLIN)
                        handleNewConnection();
                } else {
                    // Client socket
                    if (poll_fds[i].revents & POLLIN)
                        handleClientRead(fd);
                    
                    if (poll_fds[i].revents & POLLOUT)
                        handleClientWrite(fd);
                    
                    if (poll_fds[i].revents & (POLLERR | POLLHUP))
                        closeClient(fd);
                }
            }
        }
    }
};

// This is a simplified example showing the integration pattern
// Your actual implementation will need:
// - Proper socket setup and error handling
// - Configuration file parsing
// - Multiple ports/virtual hosts
// - Request timeout handling
// - CGI execution
// - And more features from the subject
