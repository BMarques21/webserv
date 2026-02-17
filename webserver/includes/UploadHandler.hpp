#ifndef UPLOADHANDLER_HPP
#define UPLOADHANDLER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <string>
#include <vector>

struct UploadedFile {
    std::string filename;
    std::string content_type;
    std::string content;
    size_t size;
};

class UploadHandler {
private:
    std::string upload_directory;
    size_t max_upload_size;
    
    bool parseMultipartFormData(const std::string& body, const std::string& boundary,
                               std::vector<UploadedFile>& files);
    bool saveFile(const std::string& filename, const std::string& content);
    std::string sanitizeFilename(const std::string& filename) const;
    bool directoryExists(const std::string& path) const;
    bool createDirectory(const std::string& path) const;
    
public:
    UploadHandler(const std::string& upload_dir, size_t max_size = 10485760); // 10MB default
    
    HttpResponse handleUpload(const HttpRequest& request);
    
    void setUploadDirectory(const std::string& dir) { upload_directory = dir; }
    void setMaxUploadSize(size_t size) { max_upload_size = size; }
    size_t getMaxUploadSize() const { return max_upload_size; }
};

#endif
