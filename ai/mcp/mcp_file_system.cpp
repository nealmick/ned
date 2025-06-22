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
        
        std::filesystem::path fsPath(expandedPath);
        
        if (!std::filesystem::exists(fsPath)) {
            return files;
        }
        
        if (!std::filesystem::is_directory(fsPath)) {
            return files;
        }
        
        for (const auto& entry : std::filesystem::directory_iterator(fsPath)) {
            std::string filename = entry.path().filename().string();
            files.push_back(filename);
        }
        
    } catch (const std::exception& e) {
    } catch (...) {
    }
    
    return files;
}

} // namespace MCP 