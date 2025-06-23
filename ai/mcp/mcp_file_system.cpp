#include "mcp_file_system.h"
#include <filesystem>

namespace MCP {

FileSystemServer::FileSystemServer() {}

std::vector<std::string> FileSystemServer::listFiles(const std::string& path) {
    std::vector<std::string> files;
    try {
        std::string expandedPath = path;
        
        // Handle tilde expansion for home directory
        if (path.length() > 0 && path[0] == '~') {
            const char* homeDir = std::getenv("HOME");
            if (homeDir) {
                expandedPath = std::string(homeDir) + path.substr(1);
            }
        }
        
        // Add trailing slash if missing (for directory paths)
        if (!expandedPath.empty() && expandedPath.back() != '/' && expandedPath.back() != '\\') {
            expandedPath += '/';
        }
        
        std::filesystem::path fsPath(expandedPath);
        
        if (!std::filesystem::exists(fsPath)) {
            // Return a special marker to indicate directory doesn't exist
            files.push_back("DIRECTORY_NOT_FOUND");
            return files;
        }
        
        if (!std::filesystem::is_directory(fsPath)) {
            // Return a special marker to indicate path is not a directory
            files.push_back("NOT_A_DIRECTORY");
            return files;
        }
        
        for (const auto& entry : std::filesystem::directory_iterator(fsPath)) {
            std::string filename = entry.path().filename().string();
            files.push_back(filename);
        }
        
        // If no files were found, the directory is empty
        if (files.empty()) {
            files.push_back("DIRECTORY_EMPTY");
        }
        
    } catch (const std::exception& e) {
        files.push_back("ERROR: " + std::string(e.what()));
    } catch (...) {
        files.push_back("ERROR: Unknown error occurred");
    }
    
    return files;
}

} // namespace MCP 