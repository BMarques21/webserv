#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <sstream>

class Utils {
public:
	// String utilities
	static std::string trim(const std::string& str);
	static std::string toLower(const std::string& str);
	static std::vector<std::string> split(const std::string& str, char delimiter);
	static std::string intToString(int value);
	static int stringToInt(const std::string& str);

	// File utilities
	static bool fileExists(const std::string& path);
	static bool isDirectory(const std::string& path);
	static std::string readFile(const std::string& path);
	static bool writeFile(const std::string& path, const std::string& content);

	// Path utilities
	static std::string getFileExtension(const std::string& path);
	static std::string joinPath(const std::string& dir, const std::string& file);
	static std::string normalizePath(const std::string& path);
};

#endif // UTILS_HPP

