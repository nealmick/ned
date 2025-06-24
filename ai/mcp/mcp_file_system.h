#pragma once

#include <string>
#include <vector>

namespace MCP {

class FileSystemServer {
public:
    FileSystemServer();
    ~FileSystemServer() = default;

    // Main method to list files in a directory
    std::vector<std::string> listFiles(const std::string& path);
    
    // Method to create a new file
    bool createFile(const std::string& path);
    
    // Method to create a new folder
    bool createFolder(const std::string& path);
    
    // Method to read file contents with sensible truncation
    std::string readFile(const std::string& path);
    
    // Method to write content to a file (overwrites existing content)
    bool writeFile(const std::string& path, const std::string& content);
};

} // namespace MCP 