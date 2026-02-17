#include "Server.hpp"
#include <iostream>
#include <cstdlib>
#include <csignal>

volatile sig_atomic_t g_shutdown = 0;

void signal_handler(int signal) {
	if (signal == SIGINT || signal == SIGTERM) {
		std::cout << "\nShutting down server..." << std::endl;
		g_shutdown = 1;
	}
}

int main(int argc, char** argv) {
	std::string config_file = "config/webserv.conf";

	if (argc > 1) {
		config_file = argv[1];
	}

	// Setup signal handlers
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGPIPE, SIG_IGN);

	try {
		Server server(config_file);
		server.run();
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
