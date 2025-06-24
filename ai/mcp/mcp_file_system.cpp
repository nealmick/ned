#include "mcp_file_system.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>

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

std::string FileSystemServer::readFile(const std::string& path) {
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
        
        // Check if file exists
        if (!std::filesystem::exists(fsPath)) {
            return "ERROR: File '" + path + "' does not exist.";
        }
        
        // Check if it's actually a file (not a directory)
        if (!std::filesystem::is_regular_file(fsPath)) {
            return "ERROR: '" + path + "' is not a regular file.";
        }
        
        // Open and read the file
        std::ifstream file(fsPath);
        if (!file.is_open()) {
            return "ERROR: Cannot open file '" + path + "' for reading.";
        }
        
        std::string content;
        std::string line;
        int lineCount = 0;
        const int maxLines = 50;
        const int maxLineLength = 500;
        
        while (std::getline(file, line) && lineCount < maxLines) {
            // Truncate lines that are too long
            if (line.length() > maxLineLength) {
                line = line.substr(0, maxLineLength) + "... [truncated]";
            }
            content += line + "\n";
            lineCount++;
        }
        
        file.close();
        
        // Add truncation notice if we stopped early
        if (lineCount >= maxLines) {
            content += "... [file truncated at " + std::to_string(maxLines) + " lines]";
        }
        
        return content;
        
    } catch (const std::exception& e) {
        return "ERROR: " + std::string(e.what());
    } catch (...) {
        return "ERROR: Unknown error occurred while reading file.";
    }
}

bool FileSystemServer::writeFile(const std::string& path, const std::string& content) {
    std::cout << "DEBUG: writeFile called with path: " << path << std::endl;
    std::cout << "DEBUG: writeFile content length: " << content.length() << std::endl;
    std::cout << "DEBUG: writeFile content (raw): [" << content << "]" << std::endl;
    
    // Enhanced content debugging - show each character and its hex value
    std::cout << "DEBUG: Content character analysis:" << std::endl;
    for (size_t i = 0; i < content.length(); ++i) {
        char c = content[i];
        if (c == '\n') {
            std::cout << "  [" << i << "]: '\\n' (0x0A)" << std::endl;
        } else if (c == '\r') {
            std::cout << "  [" << i << "]: '\\r' (0x0D)" << std::endl;
        } else if (c == '\t') {
            std::cout << "  [" << i << "]: '\\t' (0x09)" << std::endl;
        } else if (c >= 32 && c <= 126) {
            std::cout << "  [" << i << "]: '" << c << "' (0x" << std::hex << (int)c << std::dec << ")" << std::endl;
        } else {
            std::cout << "  [" << i << "]: <control> (0x" << std::hex << (int)c << std::dec << ")" << std::endl;
        }
    }
    
    try {
        std::string expandedPath = path;
        
        // Handle tilde expansion for home directory
        if (path.length() > 0 && path[0] == '~') {
            const char* homeDir = std::getenv("HOME");
            if (homeDir) {
                expandedPath = std::string(homeDir) + path.substr(1);
            }
        }
        
        std::cout << "DEBUG: writeFile expanded path: " << expandedPath << std::endl;
        
        std::filesystem::path fsPath(expandedPath);
        
        // Create parent directories if they don't exist
        std::filesystem::path parentPath = fsPath.parent_path();
        if (!parentPath.empty() && !std::filesystem::exists(parentPath)) {
            std::filesystem::create_directories(parentPath);
        }
        
        // Open file for writing (this will overwrite existing content)
        std::ofstream file(fsPath, std::ios::binary);  // Use binary mode to preserve exact content
        if (!file.is_open()) {
            std::cout << "DEBUG: writeFile failed to open file for writing" << std::endl;
            return false; // Failed to open file for writing
        }
        
        // Write the content using write() to ensure exact byte-for-byte writing
        file.write(content.c_str(), content.length());
        
        // Verify the write was successful
        if (file.fail()) {
            std::cout << "DEBUG: writeFile failed during write operation" << std::endl;
            file.close();
            return false;
        }
        
        file.close();
        
        std::cout << "DEBUG: writeFile successfully wrote " << content.length() << " characters" << std::endl;
        
        // Verify the file was written correctly by reading it back
        std::ifstream verifyFile(fsPath, std::ios::binary);
        if (verifyFile.is_open()) {
            std::string writtenContent((std::istreambuf_iterator<char>(verifyFile)),
                                      std::istreambuf_iterator<char>());
            verifyFile.close();
            
            std::cout << "DEBUG: Verification - written content length: " << writtenContent.length() << std::endl;
            std::cout << "DEBUG: Verification - content matches: " << (writtenContent == content ? "YES" : "NO") << std::endl;
            
            if (writtenContent != content) {
                std::cout << "DEBUG: WARNING - Content mismatch detected!" << std::endl;
                std::cout << "DEBUG: Original content: [" << content << "]" << std::endl;
                std::cout << "DEBUG: Written content: [" << writtenContent << "]" << std::endl;
            }
        }
        
        return true; // File written successfully
        
    } catch (const std::exception& e) {
        std::cout << "DEBUG: writeFile exception: " << e.what() << std::endl;
        return false; // Exception occurred
    } catch (...) {
        std::cout << "DEBUG: writeFile unknown error occurred" << std::endl;
        return false; // Unknown error occurred
    }
}

} // namespace MCP 