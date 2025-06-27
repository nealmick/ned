#include "mcp_file_system.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "../lib/json.hpp"
#include "../util/settings.h"
#include "../util/settings_file_manager.h"
#include <curl/curl.h>
#include "../ai_open_router.h"

using json = nlohmann::json;

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

std::string FileSystemServer::editFile(const std::string& target_file, const std::string& instructions, const std::string& code_edit) {
    try {
        std::string expandedPath = target_file;
        
        // Handle tilde expansion for home directory
        if (target_file.length() > 0 && target_file[0] == '~') {
            const char* homeDir = std::getenv("HOME");
            if (homeDir) {
                expandedPath = std::string(homeDir) + target_file.substr(1);
            }
        }
        
        std::cout << "DEBUG: editFile called with path: " << expandedPath << std::endl;
        std::cout << "DEBUG: editFile instructions: " << instructions << std::endl;
        std::cout << "DEBUG: editFile code_edit length: " << code_edit.length() << std::endl;
        
        // Check if file exists
        std::filesystem::path fsPath(expandedPath);
        if (!std::filesystem::exists(fsPath)) {
            return "ERROR: File '" + target_file + "' does not exist.";
        }
        
        // Check if it's actually a file (not a directory)
        if (!std::filesystem::is_regular_file(fsPath)) {
            return "ERROR: '" + target_file + "' is not a regular file.";
        }
        
        // Read the current file content
        std::ifstream file(fsPath);
        if (!file.is_open()) {
            return "ERROR: Cannot open file '" + target_file + "' for reading.";
        }
        
        std::string originalCode((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
        file.close();
        
        std::cout << "DEBUG: Original file content length: " << originalCode.length() << std::endl;
        
        // Get Morph API key from settings
        std::string morphApiKey;
        try {
            morphApiKey = gSettingsFileManager.getOpenRouterKey();;
        } catch (const std::exception& e) {
            return "ERROR: Failed to retrieve API key from settings: " + std::string(e.what());
        }
        
        if (morphApiKey.empty()) {
            return "ERROR: No valid API key found in settings. Please configure your OpenRouter API key.";
        }
        
        // Initialize CURL if not already done
        if (!OpenRouter::initializeCURL()) {
            return "ERROR: Failed to initialize CURL for HTTP requests.";
        }
        
        // Build the request payload for OpenRouter with Morph model
        json payload = {
            {"model", "morph/morph-v2"},
            {"messages", {
                {
                    {"role", "user"},
                    {"content", "<code>" + originalCode + "</code>\n<update>" + code_edit + "</update>"}
                }
            }},
            {"temperature", 0.0},
            {"max_tokens", 4000}
        };
        
        std::string json_str = payload.dump();
        
        // Set up CURL for the request
        CURL* curl = curl_easy_init();
        if (!curl) {
            return "ERROR: Failed to initialize CURL handle.";
        }
        
        std::string response;
        long http_code = 0;
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, ("Authorization: Bearer " + morphApiKey).c_str());
        headers = curl_slist_append(headers, "HTTP-Referer: my-text-editor");
        
        curl_easy_setopt(curl, CURLOPT_URL, "https://openrouter.ai/api/v1/chat/completions");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OpenRouter::WriteData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        
        // Perform the request
        CURLcode res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        // Cleanup CURL
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        
        if (res != CURLE_OK) {
            return "ERROR: CURL request failed: " + std::string(curl_easy_strerror(res));
        }
        
        if (http_code != 200) {
            return "ERROR: HTTP request failed with status " + std::to_string(http_code) + ": " + response;
        }
        
        std::cout << "DEBUG: OpenRouter Morph API response length: " << response.length() << std::endl;
        
        // Parse the JSON response
        try {
            json result = json::parse(response);
            if (!result.contains("choices") || !result["choices"].is_array() || result["choices"].empty()) {
                return "ERROR: Invalid response format from OpenRouter Morph API.";
            }
            
            std::string updatedCode = result["choices"][0]["message"]["content"].get<std::string>();
            
            if (updatedCode.empty()) {
                return "ERROR: Empty response from OpenRouter Morph API.";
            }
            
            std::cout << "DEBUG: Updated code length: " << updatedCode.length() << std::endl;
            
            // Write the updated content back to the file
            std::ofstream outFile(fsPath, std::ios::binary);
            if (!outFile.is_open()) {
                return "ERROR: Cannot open file '" + target_file + "' for writing.";
            }
            
            outFile.write(updatedCode.c_str(), updatedCode.length());
            outFile.close();
            
            if (outFile.fail()) {
                return "ERROR: Failed to write updated content to file '" + target_file + "'.";
            }
            
            return "SUCCESS: Successfully applied edit to " + target_file + ": " + instructions;
            
        } catch (const json::exception& e) {
            return "ERROR: Failed to parse JSON response from OpenRouter Morph API: " + std::string(e.what());
        }
        
    } catch (const std::exception& e) {
        return "ERROR: " + std::string(e.what());
    } catch (...) {
        return "ERROR: Unknown error occurred while editing file.";
    }
}

} // namespace MCP 