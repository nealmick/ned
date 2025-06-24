#pragma once

#include <string>
#include <vector>

namespace MCP {

class TerminalServer {
public:
    TerminalServer();
    ~TerminalServer() = default;

    // Main method to execute a terminal command and return its output
    std::string executeCommand(const std::string& command);
    
    // Method to execute a command and return both stdout and stderr
    std::string executeCommandWithErrorCapture(const std::string& command);
    
    // Method to execute a command in a specific working directory
    std::string executeCommandInDirectory(const std::string& command, const std::string& workingDirectory);
    
    // Method to check if a command exists in PATH
    bool commandExists(const std::string& command);
    
    // Method to get the current working directory
    std::string getCurrentWorkingDirectory();
};

} // namespace MCP 