#include "StaticFileHandler.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <unistd.h>

#define PATH_SEPARATOR '/'

StaticFileHandler::StaticFileHandler(const std::string& root, bool dir_listing, 
                                     const std::string& def_file)
    : root_directory(root), directory_listing_enabled(dir_listing), 
      default_file(def_file) {
}

std::string StaticFileHandler::getMimeType(const std::string& path) const {
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos)
        return "application/octet-stream";
    
    std::string ext = path.substr(dot_pos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".xml") return "application/xml";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png") return "image/png";
    if (ext == ".gif") return "image/gif";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".ico") return "image/x-icon";
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

bool StaticFileHandler::fileExists(const std::string& path) const {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool StaticFileHandler::isDirectory(const std::string& path) const {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0)
        return false;
    return S_ISDIR(buffer.st_mode);
}

std::string StaticFileHandler::readFile(const std::string& path, bool& success) const {
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        success = false;
        return "";
    }
    
    std::ostringstream oss;
    oss << file.rdbuf();
    success = true;
    return oss.str();
}

std::string StaticFileHandler::combinePaths(const std::string& base, 
                                           const std::string& relative) const {
    std::string result = base;
    
    // Ensure base ends with separator
    if (!result.empty() && result[result.length() - 1] != '/' && 
        result[result.length() - 1] != '\\') {
        result += PATH_SEPARATOR;
    }
    
    // Remove leading slash from relative path
    std::string rel = relative;
    if (!rel.empty() && (rel[0] == '/' || rel[0] == '\\')) {
        rel = rel.substr(1);
    }
    
    // Replace forward slashes with platform separator
    for (size_t i = 0; i < rel.length(); ++i) {
        if (rel[i] == '/' || rel[i] == '\\')
            rel[i] = PATH_SEPARATOR;
    }
    
    return result + rel;
}

bool StaticFileHandler::isPathSafe(const std::string& path) const {
    // Prevent directory traversal attacks
    if (path.find("..") != std::string::npos)
        return false;
    
    // Additional security checks could be added here
    return true;
}

std::string StaticFileHandler::generateDirectoryListing(const std::string& dir_path, 
                                                        const std::string& uri) const {
    std::ostringstream html;
    html << "<html><head><title>Index of " << uri << "</title>";
    html << "<style>"
         << "body { font-family: Arial, sans-serif; margin: 20px; }"
         << "h1 { color: #333; }"
         << "table { border-collapse: collapse; width: 100%; max-width: 800px; }"
         << "th, td { text-align: left; padding: 8px; border-bottom: 1px solid #ddd; }"
         << "th { background-color: #4CAF50; color: white; }"
         << "a { color: #0066cc; text-decoration: none; }"
         << "a:hover { text-decoration: underline; }"
         << "</style></head><body>";
    html << "<h1>Index of " << uri << "</h1>";
    html << "<table><tr><th>Name</th><th>Type</th></tr>";
    
    // Add parent directory link
    if (uri != "/") {
        html << "<tr><td><a href=\"..\">..</a></td><td>Directory</td></tr>";
    }
    
    DIR* dir = opendir(dir_path.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            std::string name = entry->d_name;
            if (name != "." && name != "..") {
                std::string full_path = combinePaths(dir_path, name);
                bool is_dir = isDirectory(full_path);
                std::string link = name;
                if (is_dir) link += "/";
                
                html << "<tr><td><a href=\"" << link << "\">" << name 
                     << (is_dir ? "/" : "") << "</a></td><td>" 
                     << (is_dir ? "Directory" : "File") << "</td></tr>";
            }
        }
        closedir(dir);
    }
    
    html << "</table></body></html>";
    return html.str();
}

HttpResponse StaticFileHandler::handleRequest(const HttpRequest& request) {
    HttpMethod method = request.getMethod();
    
    std::string uri = request.getUri();
    
    // Security check
    if (!isPathSafe(uri)) {
        return HttpResponse::badRequest("Invalid path");
    }
    
    // Build full file path
    std::string file_path = combinePaths(root_directory, uri);
    
    // Handle DELETE method
    if (method == DELETE) {
        if (!fileExists(file_path)) {
            return HttpResponse::notFound("The requested resource was not found");
        }
        
        // Prevent deletion of index files
        if (uri == "/" || uri.empty()) {
            return HttpResponse::methodNotAllowed("Cannot delete index file");
        }
        
        // Check if it's a default file (index.html, etc.)
        size_t last_slash = file_path.find_last_of('/');
        std::string filename = (last_slash != std::string::npos) ? file_path.substr(last_slash + 1) : file_path;
        if (filename == default_file || filename == "index.html" || filename == "index.htm") {
            return HttpResponse::methodNotAllowed("Cannot delete index files");
        }
        
        if (remove(file_path.c_str()) == 0) {
            return HttpResponse::ok("<html><body><h1>200 OK</h1><p>File deleted successfully</p></body></html>", "text/html");
        } else {
            // Return 403 Forbidden instead of 500 for permission errors
            return HttpResponse::methodNotAllowed("Permission denied: Cannot delete file");
        }
    }
    
    // Only handle GET and HEAD requests for static files
    if (method != GET && method != HEAD) {
        return HttpResponse::methodNotAllowed("Only GET and HEAD are allowed for static files");
    }
    
    // Check if file/directory exists
    if (!fileExists(file_path)) {
        return HttpResponse::notFound("The requested resource was not found");
    }
    
    // If it's a directory
    if (isDirectory(file_path)) {
        // Try to serve default file
        std::string index_path = combinePaths(file_path, default_file);
        if (fileExists(index_path) && !isDirectory(index_path)) {
            bool success;
            std::string content = readFile(index_path, success);
            if (success) {
                HttpResponse response = HttpResponse::ok(content, getMimeType(index_path));
                return response;
            }
        }
        
        // If no default file, check if directory listing is enabled
        if (directory_listing_enabled) {
            std::string listing = generateDirectoryListing(file_path, uri);
            return HttpResponse::ok(listing, "text/html");
        } else {
            return HttpResponse::notFound("Directory listing is disabled");
        }
    }
    
    // It's a file - read and serve it
    bool success;
    std::string content = readFile(file_path, success);
    if (!success) {
        return HttpResponse::internalServerError("Failed to read file");
    }
    
    return HttpResponse::ok(content, getMimeType(file_path));
}
