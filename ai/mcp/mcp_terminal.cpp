#include "mcp_terminal.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <array>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#endif

namespace MCP {

TerminalServer::TerminalServer() {}

std::string TerminalServer::executeCommand(const std::string& command) {
    try {
        // Use popen to execute the command and capture output
        std::array<char, 128> buffer;
        std::string result;
        
        // Execute command and capture stdout
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        if (!pipe) {
            return "ERROR: Failed to execute command '" + command + "'";
        }
        
        // Read the output
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        
        // Get the exit status
        int status = pclose(pipe.release());
        
        // Add exit status information
        if (status != 0) {
            result += "\n[Command exited with status: " + std::to_string(status) + "]";
        }
        
        return result;
        
    } catch (const std::exception& e) {
        return "ERROR: " + std::string(e.what());
    } catch (...) {
        return "ERROR: Unknown error occurred while executing command";
    }
}

std::string TerminalServer::executeCommandWithErrorCapture(const std::string& command) {
    try {
        // Redirect both stdout and stderr to capture all output
        std::string fullCommand = command + " 2>&1";
        
        std::array<char, 128> buffer;
        std::string result;
        
        // Execute command and capture both stdout and stderr
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(fullCommand.c_str(), "r"), pclose);
        if (!pipe) {
            return "ERROR: Failed to execute command '" + command + "'";
        }
        
        // Read the output
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        
        // Get the exit status
        int status = pclose(pipe.release());
        
        // Add exit status information
        if (status != 0) {
            result += "\n[Command exited with status: " + std::to_string(status) + "]";
        }
        
        return result;
        
    } catch (const std::exception& e) {
        return "ERROR: " + std::string(e.what());
    } catch (...) {
        return "ERROR: Unknown error occurred while executing command";
    }
}

std::string TerminalServer::executeCommandInDirectory(const std::string& command, const std::string& workingDirectory) {
    try {
        // Check if the working directory exists
        if (!std::filesystem::exists(workingDirectory) || !std::filesystem::is_directory(workingDirectory)) {
            return "ERROR: Working directory '" + workingDirectory + "' does not exist or is not a directory";
        }
        
        // Save current working directory
        std::string currentDir = getCurrentWorkingDirectory();
        
        // Change to the specified directory
        if (chdir(workingDirectory.c_str()) != 0) {
            return "ERROR: Failed to change to working directory '" + workingDirectory + "'";
        }
        
        // Execute the command
        std::string result = executeCommand(command);
        
        // Change back to original directory
        chdir(currentDir.c_str());
        
        return result;
        
    } catch (const std::exception& e) {
        return "ERROR: " + std::string(e.what());
    } catch (...) {
        return "ERROR: Unknown error occurred while executing command in directory";
    }
}

bool TerminalServer::commandExists(const std::string& command) {
    try {
        // Use 'which' command on Unix-like systems, 'where' on Windows
#ifdef _WIN32
        std::string checkCommand = "where " + command + " >nul 2>&1";
#else
        std::string checkCommand = "which " + command + " >/dev/null 2>&1";
#endif
        
        int result = system(checkCommand.c_str());
        return result == 0;
        
    } catch (const std::exception& e) {
        return false;
    } catch (...) {
        return false;
    }
}

std::string TerminalServer::getCurrentWorkingDirectory() {
    try {
        char buffer[FILENAME_MAX];
        if (getcwd(buffer, FILENAME_MAX) != nullptr) {
            return std::string(buffer);
        } else {
            return "ERROR: Failed to get current working directory";
        }
    } catch (const std::exception& e) {
        return "ERROR: " + std::string(e.what());
    } catch (...) {
        return "ERROR: Unknown error occurred while getting working directory";
    }
}

} // namespace MCP 