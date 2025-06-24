#include "mcp_file_system.h"
#include <filesystem>
#include <fstream>

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

bool FileSystemServer::createFile(const std::string& path) {
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
        
        // Check if file already exists
        if (std::filesystem::exists(fsPath)) {
            return false; // File already exists
        }
        
        // Create parent directories if they don't exist
        std::filesystem::path parentPath = fsPath.parent_path();
        if (!parentPath.empty() && !std::filesystem::exists(parentPath)) {
            std::filesystem::create_directories(parentPath);
        }
        
        // Create the file
        std::ofstream file(fsPath);
        if (file.is_open()) {
            file.close();
            return true; // File created successfully
        } else {
            return false; // Failed to create file
        }
        
    } catch (const std::exception& e) {
        return false; // Exception occurred
    } catch (...) {
        return false; // Unknown error occurred
    }
}

bool FileSystemServer::createFolder(const std::string& path) {
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
        
        // Check if folder already exists
        if (std::filesystem::exists(fsPath)) {
            return false; // Folder already exists
        }
        
        // Create the folder and all parent directories
        bool success = std::filesystem::create_directories(fsPath);
        return success;
        
    } catch (const std::exception& e) {
        return false; // Exception occurred
    } catch (...) {
        return false; // Unknown error occurred
    }
}

} // namespace MCP 