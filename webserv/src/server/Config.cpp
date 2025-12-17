#include "Config.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

Config::Config() {}

Config::Config(const std::string& config_file) : _config_file(config_file) {}

Config::~Config() {}

bool Config::parse() {
	// For now, create a default configuration
	// TODO: Implement actual config file parsing
	ServerConfig default_config;
	default_config.port = 8080;
	default_config.host = "0.0.0.0";
	default_config.server_name = "webserv";
	default_config.max_body_size = 1048576; // 1MB

	// Default location
	LocationConfig root_location;
	root_location.path = "/";
	root_location.root = "./www";
	root_location.index = "index.html";
	root_location.autoindex = false;
	root_location.methods.push_back("GET");
	root_location.methods.push_back("POST");
	root_location.methods.push_back("DELETE");

	default_config.locations.push_back(root_location);

	// Default error pages
	default_config.error_pages[404] = "./www/404.html";
	default_config.error_pages[500] = "./www/500.html";

	_servers.push_back(default_config);

	return true;
}

const std::vector<ServerConfig>& Config::getServers() const {
	return _servers;
}

const ServerConfig& Config::getServerConfig(size_t index) const {
	return _servers[index];
}

const LocationConfig* Config::findLocation(const std::string& uri, const ServerConfig& server) const {
	const LocationConfig* best_match = NULL;
	size_t best_match_len = 0;

	for (size_t i = 0; i < server.locations.size(); ++i) {
		const LocationConfig& loc = server.locations[i];
		if (uri.find(loc.path) == 0) {
			if (loc.path.length() > best_match_len) {
				best_match = &loc;
				best_match_len = loc.path.length();
			}
		}
	}

	return best_match;
}

void Config::_parseServerBlock(const std::string& block, ServerConfig& config) {
	// TODO: Implement actual parsing
	(void)block;
	(void)config;
}

void Config::_parseLocationBlock(const std::string& block, LocationConfig& location) {
	// TODO: Implement actual parsing
	(void)block;
	(void)location;
}

std::string Config::_trim(const std::string& str) const {
	size_t start = 0;
	size_t end = str.length();

	while (start < end && (str[start] == ' ' || str[start] == '\t' || str[start] == '\n' || str[start] == '\r')) {
		start++;
	}

	while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t' || str[end - 1] == '\n' || str[end - 1] == '\r')) {
		end--;
	}

	return str.substr(start, end - start);
}

std::vector<std::string> Config::_split(const std::string& str, char delimiter) const {
	std::vector<std::string> tokens;
	std::istringstream stream(str);
	std::string token;

	while (std::getline(stream, token, delimiter)) {
		tokens.push_back(_trim(token));
	}

	return tokens;
}

