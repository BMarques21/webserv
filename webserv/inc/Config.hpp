#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>

struct LocationConfig {
	std::string path;
	std::string root;
	std::vector<std::string> methods;
	std::string index;
	bool autoindex;
	std::string redirect;
	std::string upload_path;
	std::map<std::string, std::string> cgi_extensions; // .php -> /usr/bin/php-cgi

	LocationConfig() : autoindex(false) {}
};

struct ServerConfig {
	int port;
	std::string host;
	std::string server_name;
	size_t max_body_size;
	std::map<int, std::string> error_pages;
	std::vector<LocationConfig> locations;

	ServerConfig() : port(8080), host("0.0.0.0"), max_body_size(1048576) {} // 1MB default
};

class Config {
private:
	std::vector<ServerConfig> _servers;
	std::string _config_file;

public:
	Config();
	Config(const std::string& config_file);
	~Config();

	// Parsing
	bool parse();

	// Getters
	const std::vector<ServerConfig>& getServers() const;
	const ServerConfig& getServerConfig(size_t index) const;

	// Matching
	const LocationConfig* findLocation(const std::string& uri, const ServerConfig& server) const;

private:
	void _parseServerBlock(const std::string& block, ServerConfig& config);
	void _parseLocationBlock(const std::string& block, LocationConfig& location);
	void _parseConfigFile(const std::string& path);
	size_t _findClosingBrace(const std::string& str, size_t start) const;
	std::string _trim(const std::string& str) const;
	std::vector<std::string> _split(const std::string& str, char delimiter) const;
};

#endif // CONFIG_HPP

