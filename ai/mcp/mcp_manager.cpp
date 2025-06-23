#include "mcp_manager.h"
#include "mcp_file_system.h"
#include <sstream>
#include <regex>

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
    // Only treat as a tool call if it's a pure tool call (no additional text)
    std::string trimmed = response;
    // Trim leading and trailing whitespace
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);
    
    // Check if it starts with TOOL_CALL: (allowing for spaces) and contains no other significant text
    std::regex toolCallStartRegex(R"(\s*TOOL_CALL\s*:)");
    if (std::regex_search(trimmed, toolCallStartRegex)) {
        // Look for any text after the tool call that isn't just whitespace or newlines
        size_t newline = trimmed.find('\n');
        if (newline == std::string::npos) {
            // No newlines, so it's just the tool call
            return true;
        } else {
            // Check if there's any non-whitespace content after the first line
            std::string afterFirstLine = trimmed.substr(newline + 1);
            if (afterFirstLine.find_first_not_of(" \t\n\r") == std::string::npos) {
                // Only whitespace after the tool call
                return true;
            }
        }
    }
    return false;
}

std::string Manager::parseAndExecuteToolCalls(const std::string& llmResponse) {
    std::string result = llmResponse;
    
    // Find all tool calls in the response - more flexible regex to handle malformed calls
    // Updated regex to handle spaces around TOOL_CALL: and be more flexible
    std::regex toolCallRegex(R"(\s*TOOL_CALL\s*:\s*([^:\n]+)\s*:\s*([^:\n]+)(?:\s*=\s*([^:\n]+))?\s*)");
    std::smatch match;
    std::string::const_iterator searchStart(result.cbegin());
    
    int toolCallCount = 0;
    while (std::regex_search(searchStart, result.cend(), match, toolCallRegex)) {
        toolCallCount++;
        
        std::string toolCallStr = match.str();
        
        // Parse the tool call
        ToolCall toolCall = parseToolCall(toolCallStr);
        
        // Execute the tool call
        std::string toolResult = executeToolCall(toolCall);
        
        // Replace the tool call with the result
        size_t pos = result.find(toolCallStr);
        if (pos != std::string::npos) {
            std::string replacement = "Tool Result: " + toolResult;
            result.replace(pos, toolCallStr.length(), replacement);
            
            // Update search start position to avoid infinite loops
            searchStart = result.cbegin() + pos + replacement.length();
        }
    }
    
    return result;
}

ToolCall Manager::parseToolCall(const std::string& toolCallStr) const {
    ToolCall toolCall;
    
    // Parse format: TOOL_CALL:toolName:param1=value1:param2=value2
    // Updated regex to handle spaces around TOOL_CALL: and be more flexible
    std::regex toolCallRegex(R"(\s*TOOL_CALL\s*:\s*([^:]+)\s*:\s*(.+))");
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
        
        // Parse parameters - handle both param=value and paramvalue formats
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