#include "mcp_manager.h"
#include "mcp_file_system.h"
#include <sstream>
#include <regex>
#include <iostream>

namespace MCP {

Manager::Manager() {
    // Register default tools
    ToolDefinition listFilesTool;
    listFilesTool.name = "listFiles";
    listFilesTool.description = "List files in a directory. Supports tilde (~) expansion for home directory paths (e.g., ~/Documents, ~/ned/)";
    
    ToolParameter pathParam;
    pathParam.name = "path";
    pathParam.type = "string";
    pathParam.description = "Directory path to list files from (supports ~ for home directory)";
    pathParam.required = true;
    
    listFilesTool.parameters.push_back(pathParam);
    registerTool(listFilesTool);
    
    // Register createFile tool
    ToolDefinition createFileTool;
    createFileTool.name = "createFile";
    createFileTool.description = "Create a new empty file at the specified path. Supports tilde (~) expansion for home directory paths. Will create parent directories if they don't exist.";
    
    ToolParameter createFilePathParam;
    createFilePathParam.name = "path";
    createFilePathParam.type = "string";
    createFilePathParam.description = "File path to create (supports ~ for home directory). Parent directories will be created automatically.";
    createFilePathParam.required = true;
    
    createFileTool.parameters.push_back(createFilePathParam);
    registerTool(createFileTool);
    
    // Register createFolder tool
    ToolDefinition createFolderTool;
    createFolderTool.name = "createFolder";
    createFolderTool.description = "Create a new folder at the specified path. Supports tilde (~) expansion for home directory paths. Will create parent directories if they don't exist.";
    
    ToolParameter createFolderPathParam;
    createFolderPathParam.name = "path";
    createFolderPathParam.type = "string";
    createFolderPathParam.description = "Folder path to create (supports ~ for home directory). Parent directories will be created automatically.";
    createFolderPathParam.required = true;
    
    createFolderTool.parameters.push_back(createFolderPathParam);
    registerTool(createFolderTool);
}

void Manager::registerTool(const ToolDefinition& toolDef) {
    toolDefinitions.push_back(toolDef);
}

std::vector<ToolDefinition> Manager::getToolDefinitions() const {
    return toolDefinitions;
}

std::string Manager::getToolDescriptionsJSON() const {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < toolDefinitions.size(); ++i) {
        const auto& tool = toolDefinitions[i];
        oss << "{\"name\":\"" << tool.name << "\",";
        oss << "\"description\":\"" << tool.description << "\",";
        oss << "\"parameters\":[";
        for (size_t j = 0; j < tool.parameters.size(); ++j) {
            const auto& param = tool.parameters[j];
            oss << "{\"name\":\"" << param.name << "\",";
            oss << "\"type\":\"" << param.type << "\",";
            oss << "\"description\":\"" << param.description << "\",";
            oss << "\"required\":" << (param.required ? "true" : "false") << "}";
            if (j + 1 < tool.parameters.size()) oss << ",";
        }
        oss << "]}";
        if (i + 1 < toolDefinitions.size()) oss << ",";
    }
    oss << "]";
    return oss.str();
}

bool Manager::hasToolCalls(const std::string& response) const {
    // Look for tool calls with flexible format matching
    // Accept both TOOL_CALL: and TOOL: formats
    std::regex toolCallRegex(R"((?:TOOL_CALL|TOOL)\s*:\s*([a-zA-Z][a-zA-Z0-9]*)\s*:\s*([^:\n]+(?:\s*=\s*[^:\n]+)?(?:\s*:\s*[^:\n]+(?:\s*=\s*[^:\n]+)?)*))");
    bool found = std::regex_search(response, toolCallRegex);
    std::cout << "hasToolCalls: searching for tool calls in response" << std::endl;
    std::cout << "hasToolCalls: found = " << (found ? "true" : "false") << std::endl;
    if (found) {
        std::smatch match;
        std::regex_search(response, match, toolCallRegex);
        std::cout << "hasToolCalls: matched: " << match.str() << std::endl;
    }
    return found;
}

std::string Manager::parseAndExecuteToolCalls(const std::string& llmResponse) {
    std::string result = llmResponse;
    
    // Find all tool calls in the response with flexible format matching
    // Accept both TOOL_CALL: and TOOL: formats
    std::regex toolCallRegex(R"((?:TOOL_CALL|TOOL)\s*:\s*([a-zA-Z][a-zA-Z0-9]*)\s*:\s*([^:\n]+(?:\s*=\s*[^:\n]+)?(?:\s*:\s*[^:\n]+(?:\s*=\s*[^:\n]+)?)*))");
    std::smatch match;
    std::string::const_iterator searchStart(result.cbegin());
    
    int toolCallCount = 0;
    while (std::regex_search(searchStart, result.cend(), match, toolCallRegex)) {
        toolCallCount++;
        
        std::string toolCallStr = match.str();
        
        // Parse the tool call
        ToolCall toolCall = parseToolCall(toolCallStr);
        
        // Only execute if we have a valid tool name
        if (!toolCall.toolName.empty()) {
            // Execute the tool call
            std::string toolResult = executeToolCall(toolCall);
            
            // Replace the tool call with the result
            size_t pos = result.find(toolCallStr);
            if (pos != std::string::npos) {
                std::string replacement = "\n\n=== TOOL EXECUTION RESULT ===\n";
                replacement += "Tool: " + toolCall.toolName + "\n";
                replacement += "Result: " + toolResult + "\n";
                replacement += "=== END TOOL RESULT ===\n\n";
                result.replace(pos, toolCallStr.length(), replacement);
                
                // Update search start position to avoid infinite loops
                searchStart = result.cbegin() + pos + replacement.length();
            }
        } else {
            // Skip invalid tool calls and move search position
            size_t pos = result.find(toolCallStr);
            if (pos != std::string::npos) {
                searchStart = result.cbegin() + pos + toolCallStr.length();
            }
        }
    }
    
    return result;
}

ToolCall Manager::parseToolCall(const std::string& toolCallStr) const {
    ToolCall toolCall;
    
    // Parse format: (TOOL_CALL|TOOL):toolName:param1=value1:param2=value2
    std::regex toolCallRegex(R"((?:TOOL_CALL|TOOL)\s*:\s*([a-zA-Z][a-zA-Z0-9]*)\s*:\s*(.+))");
    std::smatch match;
    
    if (std::regex_match(toolCallStr, match, toolCallRegex)) {
        toolCall.toolName = match[1].str();
        // Trim whitespace from tool name
        toolCall.toolName.erase(0, toolCall.toolName.find_first_not_of(" \t"));
        toolCall.toolName.erase(toolCall.toolName.find_last_not_of(" \t") + 1);
        
        std::string paramsStr = match[2].str();
        // Trim whitespace from params string
        paramsStr.erase(0, paramsStr.find_first_not_of(" \t"));
        paramsStr.erase(paramsStr.find_last_not_of(" \t") + 1);
        
        // Parse parameters - split by : and then by = for each parameter
        std::regex paramRegex(R"(([^=\s]+)\s*=\s*([^:\s]+))");
        std::sregex_iterator paramIter(paramsStr.begin(), paramsStr.end(), paramRegex);
        std::sregex_iterator paramEnd;
        
        for (; paramIter != paramEnd; ++paramIter) {
            std::string paramName = (*paramIter)[1].str();
            std::string paramValue = (*paramIter)[2].str();
            
            // Trim whitespace from parameter name and value
            paramName.erase(0, paramName.find_first_not_of(" \t"));
            paramName.erase(paramName.find_last_not_of(" \t") + 1);
            paramValue.erase(0, paramValue.find_first_not_of(" \t"));
            paramValue.erase(paramValue.find_last_not_of(" \t") + 1);
            
            toolCall.parameters[paramName] = paramValue;
        }
    }
    
    return toolCall;
}

std::string Manager::executeToolCall(const ToolCall& toolCall) const {
    if (toolCall.toolName == "listFiles") {
        auto it = toolCall.parameters.find("path");
        if (it != toolCall.parameters.end()) {
            FileSystemServer fsServer;
            auto files = fsServer.listFiles(it->second);
            
            // Check for special error markers
            if (!files.empty()) {
                if (files[0] == "DIRECTORY_NOT_FOUND") {
                    return "Error: Directory '" + it->second + "' does not exist.";
                } else if (files[0] == "NOT_A_DIRECTORY") {
                    return "Error: '" + it->second + "' is not a directory.";
                } else if (files[0] == "DIRECTORY_EMPTY") {
                    return "Directory '" + it->second + "' is empty.";
                } else if (files[0].substr(0, 6) == "ERROR:") {
                    return files[0]; // Return the error message as-is
                }
            }
            
            // Normal case - directory exists and has files
            std::ostringstream result;
            result << "Files in " << it->second << ":\n";
            for (const auto& file : files) {
                result << "- " << file << "\n";
            }
            std::string resultStr = result.str();
            return resultStr;
        } else {
            return "Error: Missing 'path' parameter for listFiles tool";
        }
    } else if (toolCall.toolName == "createFile") {
        auto it = toolCall.parameters.find("path");
        if (it != toolCall.parameters.end()) {
            FileSystemServer fsServer;
            bool success = fsServer.createFile(it->second);
            
            if (success) {
                return "Success: File '" + it->second + "' created successfully.";
            } else {
                return "Error: Failed to create file '" + it->second + "'. The file may already exist or there may be permission issues.";
            }
        } else {
            return "Error: Missing 'path' parameter for createFile tool";
        }
    } else if (toolCall.toolName == "createFolder") {
        auto it = toolCall.parameters.find("path");
        if (it != toolCall.parameters.end()) {
            FileSystemServer fsServer;
            bool success = fsServer.createFolder(it->second);
            
            if (success) {
                return "Success: Folder '" + it->second + "' created successfully.";
            } else {
                return "Error: Failed to create folder '" + it->second + "'. The folder may already exist or there may be permission issues.";
            }
        } else {
            return "Error: Missing 'path' parameter for createFolder tool";
        }
    }
    
    return "Error: Unknown tool '" + toolCall.toolName + "'";
}

const ToolDefinition* Manager::getToolDefinition(const std::string& name) const {
    for (const auto& tool : toolDefinitions) {
        if (tool.name == name) {
            return &tool;
        }
    }
    return nullptr;
}

bool Manager::routeToolCall(const std::string& toolName, const std::unordered_map<std::string, std::string>& params) {
    // This method is now deprecated in favor of parseAndExecuteToolCalls
    return false;
}

} // namespace MCP 

// Global MCP manager instance
MCP::Manager gMCPManager; 