#include "Config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>

Config::Config() {}

Config::Config(const std::string& config_file) : _config_file(config_file) {}

Config::~Config() {}

bool Config::parse() {
	// If config file is specified, parse it
	if (!_config_file.empty()) {
		try {
			_parseConfigFile(_config_file);
			return !_servers.empty();
		} catch (const std::exception& e) {
			std::cerr << "Config parse error: " << e.what() << std::endl;
			// Fall back to default configuration
		}
	}

	// Create default configuration if parsing failed or no config file specified
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

// Extract server-level directives and location blocks
void Config::_parseServerBlock(const std::string& block, ServerConfig& config) {
	std::vector<std::string> lines = _split(block, '\n');

	for (size_t i = 0; i < lines.size(); ++i)
	{
		std::string line = _trim(lines[i]);
		if (line.empty() || line[0] == '#') continue;

		if (line.find("listen") == 0)
		{
			std::vector<std::string> tokens = _split(line, ' ');
			if (tokens.size() >= 2)
			{
				std::string port_str = tokens[1];
				if (port_str[port_str.length() - 1] == ';')
					port_str = port_str.substr(0, port_str.length() - 1);
				config.port = std::atoi(port_str.c_str());
			}
		}
		else if (line.find("host") == 0)
		{
			std::vector<std::string> tokens = _split(line, ' ');
			if (tokens.size() >= 2)
			{
				config.host = tokens[1];
				if (config.host[config.host.length() - 1] == ';')
					config.host = config.host.substr(0, config.host.length() - 1);
			}
		}
		else if (line.find("server_name") == 0)
		{
			std::vector<std::string> tokens = _split(line, ' ');
			if (tokens.size() >= 2)
			{
				config.server_name = tokens[1];
				if (config.server_name[config.server_name.length() - 1] == ';')
					config.server_name = config.server_name.substr(0, config.server_name.length() - 1);
			}
		}
		else if (line.find("max_body_size") == 0)
		{
			std::vector<std::string> tokens = _split(line, ' ');
			if (tokens.size() >= 2)
			{
				std::string size_str = tokens[1];
				if (size_str[size_str.length() - 1] == ';')
					size_str = size_str.substr(0, size_str.length() - 1);
				config.max_body_size = std::atoi(size_str.c_str());
			}
		}
		else if (line.find("error_page") == 0)
		{
			std::vector<std::string> tokens = _split(line, ' ');
			if (tokens.size() >= 3)
			{
				int error_code = std::atoi(tokens[1].c_str());
				std::string error_path = tokens[2];
				if (error_path[error_path.length() - 1] == ';')
					error_path = error_path.substr(0, error_path.length() - 1);
				config.error_pages[error_code] = error_path;
			}
		}
		else if (line.find("location") == 0)
		{
			size_t start = block.find("{", i);
			size_t end = _findClosingBrace(block, start);
			std::string loc_block = block.substr(start + 1, end - start - 1);

			std::vector<std::string> tokens = _split(line, ' ');
			LocationConfig location;
			if (tokens.size() >= 2)
				location.path = tokens[1];

			_parseLocationBlock(loc_block, location);
			config.locations.push_back(location);
		}
	}
}

// Extract location-specific directives
void Config::_parseLocationBlock(const std::string& block, LocationConfig& location) {
	std::vector<std::string> lines = _split(block, '\n');

	for (size_t i = 0; i < lines.size(); ++i)
	{
		std::string line = _trim(lines[i]);
		if (line.empty() || line[0] == '#') continue;

		if (line.find("root") == 0)
		{
			std::vector<std::string> tokens = _split(line, ' ');
			if (tokens.size() >= 2)
			{
				location.root = tokens[1];
				if (location.root[location.root.length() - 1] == ';')
					location.root = location.root.substr(0, location.root.length() - 1);
			}
		}
		else if (line.find("index") == 0)
		{
			std::vector<std::string> tokens = _split(line, ' ');
			if (tokens.size() >= 2)
			{
				location.index = tokens[1];
				if (location.index[location.index.length() - 1] == ';')
					location.index = location.index.substr(0, location.index.length() - 1);
			}
		}
		else if (line.find("autoindex") == 0)
		{
			std::vector<std::string> tokens = _split(line, ' ');
			if (tokens.size() >= 2)
			{
				std::string value = tokens[1];
				if (value[value.length() - 1] == ';')
					value = value.substr(0, value.length() - 1);
				location.autoindex = (value == "on");
			}
		}
		else if (line.find("methods") == 0)
		{
			std::vector<std::string> tokens = _split(line, ' ');
			for (size_t j = 1; j < tokens.size(); ++j)
			{
				std::string method = tokens[j];
				if (method[method.length() - 1] == ';')
					method = method.substr(0, method.length() - 1);
				if (!method.empty())
					location.methods.push_back(method);
			}
		}
		else if (line.find("upload_path") == 0)
		{
			std::vector<std::string> tokens = _split(line, ' ');
			if (tokens.size() >= 2)
			{
				location.upload_path = tokens[1];
				if (location.upload_path[location.upload_path.length() - 1] == ';')
					location.upload_path = location.upload_path.substr(0, location.upload_path.length() - 1);
			}
		}
		else if (line.find("redirect") == 0)
		{
			std::vector<std::string> tokens = _split(line, ' ');
			if (tokens.size() >= 2)
			{
				location.redirect = tokens[1];
				if (location.redirect[location.redirect.length() - 1] == ';')
					location.redirect = location.redirect.substr(0, location.redirect.length() - 1);
			}
		}
		else if (line.find("cgi") == 0)
		{
			std::vector<std::string> tokens = _split(line, ' ');
			if (tokens.size() >= 3)
			{
				std::string extension = tokens[1];
				std::string path = tokens[2];
				if (path[path.length() - 1] == ';')
					path = path.substr(0, path.length() - 1);
				location.cgi_extensions[extension] = path;
			}
		}
	}
}

void Config::_parseConfigFile(const std::string& path){
	std::ifstream file(path.c_str());
	if (!file.is_open()) {
		throw std::runtime_error("Cannot open config file: " + path);
	}

	std::string content;
	std::string line;
	while (std::getline(file, line)) {
		content += line + "\n";
	}
	file.close();

	// Find all server blocks
	size_t pos = 0;
	while ((pos = content.find("server", pos)) != std::string::npos) {
		size_t block_start = content.find("{", pos);
		if (block_start == std::string::npos) break;

		size_t block_end = _findClosingBrace(content, block_start);
		if (block_end == std::string::npos) break;

		std::string server_block = content.substr(block_start + 1, block_end - block_start - 1);
		ServerConfig config;
		_parseServerBlock(server_block, config);
		_servers.push_back(config);

		pos = block_end + 1;
	}
}

size_t Config::_findClosingBrace(const std::string& str, size_t start) const
{
	int depth = 1;
	for (size_t i = start + 1; i < str.length(); ++i)
	{
		if (str[i] == '{') depth++;
		else if (str[i] == '}') depth--;
		if (depth == 0) return i;
	}
	return std::string::npos;
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

