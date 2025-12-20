#include "Utils.hpp"
#include <algorithm>
#include <fstream>
#include <sys/stat.h>

// String utilities
std::string Utils::trim(const std::string& str) {
	size_t start = 0;
	size_t end = str.length();

	while (start < end && (str[start] == ' ' || str[start] == '\t' ||
	                       str[start] == '\n' || str[start] == '\r')) {
		start++;
	}

	while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t' ||
	                       str[end - 1] == '\n' || str[end - 1] == '\r')) {
		end--;
	}

	return str.substr(start, end - start);
}

std::string Utils::toLower(const std::string& str) {
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}

std::vector<std::string> Utils::split(const std::string& str, char delimiter) {
	std::vector<std::string> tokens;
	std::istringstream stream(str);
	std::string token;

	while (std::getline(stream, token, delimiter)) {
		tokens.push_back(trim(token));
	}

	return tokens;
}

std::string Utils::intToString(int value) {
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

int Utils::stringToInt(const std::string& str) {
	std::istringstream iss(str);
	int value;
	iss >> value;
	return value;
}

// File utilities
bool Utils::fileExists(const std::string& path) {
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0);
}

bool Utils::isDirectory(const std::string& path) {
	struct stat buffer;
	if (stat(path.c_str(), &buffer) != 0) {
		return false;
	}
	return S_ISDIR(buffer.st_mode);
}

std::string Utils::readFile(const std::string& path) {
	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file.is_open()) {
		return "";
	}

	std::ostringstream contents;
	contents << file.rdbuf();
	return contents.str();
}

bool Utils::writeFile(const std::string& path, const std::string& content) {
	std::ofstream file(path.c_str(), std::ios::binary);
	if (!file.is_open()) {
		return false;
	}

	file << content;
	return file.good();
}

// Path utilities
std::string Utils::getFileExtension(const std::string& path) {
	size_t dot_pos = path.find_last_of('.');
	if (dot_pos == std::string::npos) {
		return "";
	}
	return path.substr(dot_pos);
}

std::string Utils::joinPath(const std::string& dir, const std::string& file) {
	if (dir.empty()) {
		return file;
	}
	if (file.empty()) {
		return dir;
	}

	std::string result = dir;
	if (result[result.length() - 1] != '/') {
		result += "/";
	}

	// Skip leading slashes in file
	size_t start = 0;
	while (start < file.length() && file[start] == '/') {
		start++;
	}

	result += file.substr(start);
	return result;
}

std::string Utils::normalizePath(const std::string& path) {
	// Remove redundant slashes and normalize path
	std::string normalized;
	bool lastWasSlash = false;

	for (size_t i = 0; i < path.length(); ++i) {
		if (path[i] == '/') {
			if (!lastWasSlash) {
				normalized += '/';
				lastWasSlash = true;
			}
		} else {
			normalized += path[i];
			lastWasSlash = false;
		}
	}

	return normalized;
}

