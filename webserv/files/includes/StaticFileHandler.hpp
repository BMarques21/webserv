#ifndef STATICFILEHANDLER_HPP
#define STATICFILEHANDLER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <string>

class StaticFileHandler {
private:
    std::string root_directory;
    bool directory_listing_enabled;
    std::string default_file;
    
    std::string getMimeType(const std::string& path) const;
    std::string readFile(const std::string& path, bool& success) const;
    bool fileExists(const std::string& path) const;
    bool isDirectory(const std::string& path) const;
    std::string generateDirectoryListing(const std::string& path, const std::string& uri) const;
    std::string combinePaths(const std::string& base, const std::string& relative) const;
    bool isPathSafe(const std::string& path) const;
    
public:
    StaticFileHandler(const std::string& root, bool dir_listing = false, 
                     const std::string& default_file = "index.html");
    
    HttpResponse handleRequest(const HttpRequest& request);
    
    void setRootDirectory(const std::string& root) { root_directory = root; }
    void setDirectoryListing(bool enabled) { directory_listing_enabled = enabled; }
    void setDefaultFile(const std::string& file) { default_file = file; }
};

#endif
