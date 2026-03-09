#include "UploadHandler.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>

#ifdef _WIN32
    #include <direct.h>
    #include <windows.h>
    #define mkdir(path, mode) _mkdir(path)
    #define PATH_SEPARATOR '\\'
#else
    #include <unistd.h>
    #define PATH_SEPARATOR '/'
#endif

UploadHandler::UploadHandler(const std::string& upload_dir, size_t max_size)
    : upload_directory(upload_dir), max_upload_size(max_size) {
    
    // Ensure upload directory exists
    if (!directoryExists(upload_directory)) {
        createDirectory(upload_directory);
    }
}

bool UploadHandler::directoryExists(const std::string& path) const {
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
        return false;
    return (info.st_mode & S_IFDIR) != 0;
}

bool UploadHandler::createDirectory(const std::string& path) const {
    return mkdir(path.c_str(), 0755) == 0;
}

std::string UploadHandler::sanitizeFilename(const std::string& filename) const {
    std::string safe_name = filename;
    
    // Remove path separators and parent directory references
    size_t last_slash = safe_name.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        safe_name = safe_name.substr(last_slash + 1);
    }
    
    // Remove dangerous characters
    std::string result;
    for (size_t i = 0; i < safe_name.length(); ++i) {
        char c = safe_name[i];
        if (std::isalnum(c) || c == '.' || c == '_' || c == '-') {
            result += c;
        } else {
            result += '_';
        }
    }
    
    // Ensure filename is not empty
    if (result.empty() || result == "." || result == "..") {
        result = "uploaded_file";
    }
    
    return result;
}

bool UploadHandler::saveFile(const std::string& filename, const std::string& content) {
    std::string full_path = upload_directory;
    if (!full_path.empty() && full_path[full_path.length() - 1] != PATH_SEPARATOR) {
        full_path += PATH_SEPARATOR;
    }
    full_path += filename;
    
    std::ofstream file(full_path.c_str(), std::ios::binary);
    if (!file.is_open())
        return false;
    
    file.write(content.c_str(), content.length());
    file.close();
    
    return true;
}

bool UploadHandler::parseMultipartFormData(const std::string& body, 
                                          const std::string& boundary,
                                          std::vector<UploadedFile>& files) {
    std::string delimiter = "--" + boundary;
    std::string end_delimiter = "--" + boundary + "--";
    
    size_t pos = 0;
    while (pos < body.length()) {
        // Find next boundary
        size_t boundary_pos = body.find(delimiter, pos);
        if (boundary_pos == std::string::npos)
            break;
        
        // Move past the boundary and CRLF
        pos = boundary_pos + delimiter.length();
        
        // Check for end delimiter
        if (pos + 2 <= body.length() && body.substr(pos, 2) == "--")
            break;
        
        // Skip CRLF after boundary
        if (pos + 2 <= body.length() && body.substr(pos, 2) == "\r\n")
            pos += 2;
        
        // Find the empty line that separates headers from content
        size_t headers_end = body.find("\r\n\r\n", pos);
        if (headers_end == std::string::npos)
            break;
        
        std::string headers_section = body.substr(pos, headers_end - pos);
        pos = headers_end + 4; // Skip \r\n\r\n
        
        // Find the next boundary to get content length
        size_t next_boundary = body.find(delimiter, pos);
        if (next_boundary == std::string::npos)
            break;
        
        // Content is between current pos and next boundary (minus \r\n before boundary)
        size_t content_end = next_boundary - 2;
        if (content_end <= pos)
            break;
        
        std::string content = body.substr(pos, content_end - pos);
        
        // Parse headers to extract filename and content-type
        UploadedFile file;
        file.content = content;
        file.size = content.length();
        file.content_type = "application/octet-stream";
        
        // Parse Content-Disposition header
        size_t cd_pos = headers_section.find("Content-Disposition:");
        if (cd_pos != std::string::npos) {
            size_t line_end = headers_section.find("\r\n", cd_pos);
            std::string cd_line = headers_section.substr(cd_pos, 
                line_end == std::string::npos ? std::string::npos : line_end - cd_pos);
            
            // Extract filename
            size_t filename_pos = cd_line.find("filename=\"");
            if (filename_pos != std::string::npos) {
                filename_pos += 10; // length of 'filename="'
                size_t filename_end = cd_line.find("\"", filename_pos);
                if (filename_end != std::string::npos) {
                    file.filename = cd_line.substr(filename_pos, filename_end - filename_pos);
                }
            }
        }
        
        // Parse Content-Type header
        size_t ct_pos = headers_section.find("Content-Type:");
        if (ct_pos != std::string::npos) {
            size_t line_end = headers_section.find("\r\n", ct_pos);
            std::string ct_line = headers_section.substr(ct_pos + 13, // length of 'Content-Type:'
                line_end == std::string::npos ? std::string::npos : line_end - ct_pos - 13);
            
            // Trim whitespace
            size_t first = ct_line.find_first_not_of(" \t");
            size_t last = ct_line.find_last_not_of(" \t\r\n");
            if (first != std::string::npos && last != std::string::npos) {
                file.content_type = ct_line.substr(first, last - first + 1);
            }
        }
        
        if (!file.filename.empty()) {
            files.push_back(file);
        }
        
        pos = next_boundary;
    }
    
    return !files.empty();
}

HttpResponse UploadHandler::handleUpload(const HttpRequest& request) {
    // Only handle POST requests
    if (request.getMethod() != POST) {
        return HttpResponse::methodNotAllowed("Only POST is allowed for uploads");
    }
    
    // Check content length
    if (request.getContentLength() > max_upload_size) {
        std::ostringstream oss;
        oss << "Upload size exceeds maximum allowed size of " 
            << max_upload_size << " bytes";
        return HttpResponse::payloadTooLarge(oss.str());
    }
    
    // Get boundary from request
    std::string boundary = request.getBoundary();
    if (boundary.empty()) {
        return HttpResponse::badRequest("Missing boundary in multipart/form-data");
    }
    
    // Parse multipart form data
    std::vector<UploadedFile> files;
    if (!parseMultipartFormData(request.getBody(), boundary, files)) {
        return HttpResponse::badRequest("Failed to parse multipart/form-data");
    }
    
    // Save all files
    std::vector<std::string> saved_files;
    for (size_t i = 0; i < files.size(); ++i) {
        std::string safe_filename = sanitizeFilename(files[i].filename);
        if (saveFile(safe_filename, files[i].content)) {
            saved_files.push_back(safe_filename);
        }
    }
    
    if (saved_files.empty()) {
        return HttpResponse::internalServerError("Failed to save uploaded files");
    }
    
    // Build response
    std::ostringstream response_body;
    response_body << "<html><body><h1>Upload Successful</h1>";
    response_body << "<p>Uploaded " << saved_files.size() << " file(s):</p><ul>";
    for (size_t i = 0; i < saved_files.size(); ++i) {
        response_body << "<li>" << saved_files[i] << "</li>";
    }
    response_body << "</ul></body></html>";
    
    return HttpResponse::ok(response_body.str(), "text/html");
}
