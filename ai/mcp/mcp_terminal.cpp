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


}