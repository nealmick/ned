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
};

} // namespace MCP 