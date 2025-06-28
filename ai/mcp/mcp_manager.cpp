#include "mcp_manager.h"
#include "mcp_file_system.h"
#include "mcp_terminal.h"
#include "../lib/json.hpp"
#include <sstream>
#include <regex>
#include <iostream>

using json = nlohmann::json;

namespace MCP {

Manager::Manager() {
    // Register default tools
    ToolDefinition listFilesTool;
    listFilesTool.name = "listFiles";
    listFilesTool.description = "List files in a directory. Use this when the user asks about what files exist, wants to explore a directory, or needs to see the project structure. Supports tilde (~) expansion for home directory paths (e.g., ~/Documents, ~/ned/)";
    
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
    createFileTool.description = "Create a new empty file at the specified path. Use this when the user wants to create a new file, set up a new project file, or create placeholder files. Supports tilde (~) expansion for home directory paths. Will create parent directories if they don't exist.";
    
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
    createFolderTool.description = "Create a new folder at the specified path. Use this when the user wants to create a new directory, set up project folders, or organize files. Supports tilde (~) expansion for home directory paths. Will create parent directories if they don't exist.";
    
    ToolParameter createFolderPathParam;
    createFolderPathParam.name = "path";
    createFolderPathParam.type = "string";
    createFolderPathParam.description = "Folder path to create (supports ~ for home directory). Parent directories will be created automatically.";
    createFolderPathParam.required = true;
    
    createFolderTool.parameters.push_back(createFolderPathParam);
    registerTool(createFolderTool);
    
    // Register readFile tool
    ToolDefinition readFileTool;
    readFileTool.name = "readFile";
    readFileTool.description = "Read the contents of a file. Use this when the user asks to see file contents, wants to examine code, or needs to understand what's in a file before making changes. Supports tilde (~) expansion for home directory paths. Returns first 50 lines with lines truncated to 500 characters maximum.";
    
    ToolParameter readFilePathParam;
    readFilePathParam.name = "path";
    readFilePathParam.type = "string";
    readFilePathParam.description = "File path to read (supports ~ for home directory)";
    readFilePathParam.required = true;
    
    readFileTool.parameters.push_back(readFilePathParam);
    registerTool(readFileTool);
    
    // Register writeFile tool
    ToolDefinition writeFileTool;
    writeFileTool.name = "writeFile";
    writeFileTool.description = "Write content to a file, overwriting existing content. Use this when the user wants to create a new file with specific content or completely replace the contents of an existing file. Supports tilde (~) expansion for home directory paths. Will create parent directories if they don't exist.";
    
    ToolParameter writeFilePathParam;
    writeFilePathParam.name = "path";
    writeFilePathParam.type = "string";
    writeFilePathParam.description = "File path to write to (supports ~ for home directory). Parent directories will be created automatically.";
    writeFilePathParam.required = true;
    
    ToolParameter writeFileContentParam;
    writeFileContentParam.name = "content";
    writeFileContentParam.type = "string";
    writeFileContentParam.description = "Content to write to the file";
    writeFileContentParam.required = true;
    
    writeFileTool.parameters.push_back(writeFilePathParam);
    writeFileTool.parameters.push_back(writeFileContentParam);
    registerTool(writeFileTool);
    
    // Register editFile tool
    ToolDefinition editFileTool;
    editFileTool.name = "editFile";
    editFileTool.description = "Use this tool to propose an edit to an existing file using Morph's fast apply API. This will be read by a less intelligent model, which will quickly apply the edit. You should make it clear what the edit is, while also minimizing the unchanged code you write. When writing the edit, you should specify each edit in sequence, with the special comment // ... existing code ... to represent unchanged code in between edited lines. You should bias towards repeating as few lines of the original file as possible to convey the change. NEVER show unmodified code in the edit, unless sufficient context of unchanged lines around the code you're editing is needed to resolve ambiguity. If you plan on deleting a section, you must provide surrounding context to indicate the deletion. DO NOT omit spans of pre-existing code without using the // ... existing code ... comment to indicate its absence.";
    
    ToolParameter editFilePathParam;
    editFilePathParam.name = "target_file";
    editFilePathParam.type = "string";
    editFilePathParam.description = "The target file to modify. Always specify the target file as the first argument and use the relative path in the workspace of the file to edit";
    editFilePathParam.required = true;
    
    ToolParameter editFileInstructionsParam;
    editFileInstructionsParam.name = "instructions";
    editFileInstructionsParam.type = "string";
    editFileInstructionsParam.description = "A single sentence instruction describing the edit in first person";
    editFileInstructionsParam.required = true;
    
    ToolParameter editFileCodeEditParam;
    editFileCodeEditParam.name = "code_edit";
    editFileCodeEditParam.type = "string";
    editFileCodeEditParam.description = "Specify ONLY the precise lines of code that you wish to edit. NEVER specify or write out unchanged code. Instead, represent all unchanged code using the comment of the language you're editing in - example: // ... existing code ...";
    editFileCodeEditParam.required = true;
    
    editFileTool.parameters.push_back(editFilePathParam);
    editFileTool.parameters.push_back(editFileInstructionsParam);
    editFileTool.parameters.push_back(editFileCodeEditParam);
    registerTool(editFileTool);
    
    // Register terminal tools
    ToolDefinition executeCommandTool;
    executeCommandTool.name = "executeCommand";
    executeCommandTool.description = "Execute a terminal command and return its output. Use this for most command execution needs like running build commands, checking system status, or running scripts. Captures stdout and shows exit status.";
    
    ToolParameter commandParam;
    commandParam.name = "command";
    commandParam.type = "string";
    commandParam.description = "The terminal command to execute";
    commandParam.required = true;
    
    executeCommandTool.parameters.push_back(commandParam);
    registerTool(executeCommandTool);
    
    // Register executeCommandWithErrorCapture tool
    ToolDefinition executeCommandWithErrorTool;
    executeCommandWithErrorTool.name = "executeCommandWithErrorCapture";
    executeCommandWithErrorTool.description = "Execute a terminal command and return both stdout and stderr output. Use this when you need to see error messages, debug command failures, or when the command might produce important output on stderr. Captures all output and shows exit status.";
    
    ToolParameter commandWithErrorParam;
    commandWithErrorParam.name = "command";
    commandWithErrorParam.type = "string";
    commandWithErrorParam.description = "The terminal command to execute";
    commandWithErrorParam.required = true;
    
    executeCommandWithErrorTool.parameters.push_back(commandWithErrorParam);
    registerTool(executeCommandWithErrorTool);
    
    // Register executeCommandInDirectory tool
    ToolDefinition executeCommandInDirTool;
    executeCommandInDirTool.name = "executeCommandInDirectory";
    executeCommandInDirTool.description = "Execute a terminal command in a specific working directory and return its output. Use this when you need to run commands in a particular directory context, like building a project in its directory or running tests from a specific folder.";
    
    ToolParameter commandInDirParam;
    commandInDirParam.name = "command";
    commandInDirParam.type = "string";
    commandInDirParam.description = "The terminal command to execute";
    commandInDirParam.required = true;
    
    ToolParameter workingDirParam;
    workingDirParam.name = "workingDirectory";
    workingDirParam.type = "string";
    workingDirParam.description = "The working directory to execute the command in";
    workingDirParam.required = true;
    
    executeCommandInDirTool.parameters.push_back(commandInDirParam);
    executeCommandInDirTool.parameters.push_back(workingDirParam);
    registerTool(executeCommandInDirTool);
    
    // Register commandExists tool
    ToolDefinition commandExistsTool;
    commandExistsTool.name = "commandExists";
    commandExistsTool.description = "Check if a command exists in the system PATH. Use this when you need to verify if a tool or command is available before trying to use it, or when the user asks about system capabilities.";
    
    ToolParameter commandExistsParam;
    commandExistsParam.name = "command";
    commandExistsParam.type = "string";
    commandExistsParam.description = "The command name to check";
    commandExistsParam.required = true;
    
    commandExistsTool.parameters.push_back(commandExistsParam);
    registerTool(commandExistsTool);
    
    // Register getCurrentWorkingDirectory tool
    ToolDefinition getCwdTool;
    getCwdTool.name = "getCurrentWorkingDirectory";
    getCwdTool.description = "Get the current working directory path. Use this when the user asks about their current location, needs to understand the context of relative paths, or when you need to know where commands will be executed.";
    
    registerTool(getCwdTool);
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
    // Look for tool calls with custom parsing to handle multiline content
    bool found = (response.find("TOOL_CALL:") != std::string::npos) || 
                 (response.find("TOOL:") != std::string::npos);
    
    std::cout << "hasToolCalls: searching for tool calls in response" << std::endl;
    std::cout << "hasToolCalls: response length: " << response.length() << std::endl;
    std::cout << "hasToolCalls: found = " << (found ? "true" : "false") << std::endl;
    
    if (found) {
        std::cout << "hasToolCalls: found TOOL_CALL or TOOL pattern in response" << std::endl;
    } else {
        std::cout << "hasToolCalls: no tool call found in response" << std::endl;
    }
    return found;
}

std::string Manager::processToolCalls(const std::string& llmResponse) const {
    std::string result = llmResponse;
    
    // Check if response contains tool calls
    bool found = (llmResponse.find("TOOL_CALL:") != std::string::npos) ||
                 (llmResponse.find("TOOL:") != std::string::npos) ||
                 (llmResponse.find("[TOOL_CALLS_DETECTED]") != std::string::npos);
    
    if (!found) {
        return result;
    }
    
    std::cout << "DEBUG: Tool call markers found in response" << std::endl;
    
    // Extract and process complete tool calls from the full response
    std::string::size_type pos = 0;
    
    while (pos < result.length()) {
        // Look for TOOL_CALL: patterns
        std::string::size_type toolStart = result.find("TOOL_CALL:", pos);
        if (toolStart == std::string::npos) {
            break; // No more tool calls found
        }
        
        // Find the end of this tool call (next TOOL_CALL: or [TOOL_CALLS_DETECTED] or end of string)
        std::string::size_type nextToolStart = result.find("TOOL_CALL:", toolStart + 1);
        std::string::size_type toolCallsDetected = result.find("[TOOL_CALLS_DETECTED]", toolStart + 1);
        
        std::string::size_type toolEnd;
        if (nextToolStart != std::string::npos && toolCallsDetected != std::string::npos) {
            toolEnd = std::min(nextToolStart, toolCallsDetected);
        } else if (nextToolStart != std::string::npos) {
            toolEnd = nextToolStart;
        } else if (toolCallsDetected != std::string::npos) {
            toolEnd = toolCallsDetected;
        } else {
            toolEnd = result.length();
        }
        
        // Extract the complete tool call string
        std::string toolCallStr = result.substr(toolStart, toolEnd - toolStart);
        std::cout << "DEBUG: Found tool call: [" << toolCallStr << "]" << std::endl;
        
        // Parse the tool call
        ToolCall toolCall = parseToolCall(toolCallStr);
        
        // Only execute if we have a valid tool name
        if (!toolCall.toolName.empty()) {
            std::cout << "DEBUG: Executing tool: " << toolCall.toolName << std::endl;
            for (const auto& [key, value] : toolCall.parameters) {
                std::cout << "DEBUG: Parameter: " << key << " = " << value << std::endl;
            }
            
            // Execute the tool call
            std::string toolResult = executeToolCall(toolCall);
            
            // Replace the tool call with the result
            result.replace(toolStart, toolCallStr.length(), 
                "\n\n=== TOOL EXECUTION RESULT ===\n"
                "Tool: " + toolCall.toolName + "\n"
                "Result: " + toolResult + "\n"
                "=== END TOOL RESULT ===\n\n");
            
            // Update position to continue searching
            pos = toolStart + 1;
        } else {
            // Skip invalid tool calls and move search position
            pos = toolEnd;
        }
    }
    
    // Remove the [TOOL_CALLS_DETECTED] marker
    std::string::size_type detectedPos = result.find("[TOOL_CALLS_DETECTED]");
    if (detectedPos != std::string::npos) {
        result.erase(detectedPos, 20); // Remove "[TOOL_CALLS_DETECTED]"
    }
    
    return result;
}

ToolCall Manager::parseToolCall(const std::string& toolCallStr) const {
    ToolCall toolCall;
    
    std::cout << "DEBUG: Parsing tool call: " << toolCallStr << std::endl;
    
    // Check if this is a JSON format tool call (new format)
    if (toolCallStr.find("TOOL_CALL:") == 0) {
        std::string jsonPart = toolCallStr.substr(10); // Remove "TOOL_CALL:" prefix
        
        try {
            json toolCallJson = json::parse(jsonPart);
            
            // Extract function name and arguments
            if (toolCallJson.contains("function")) {
                const auto& func = toolCallJson["function"];
                if (func.contains("name")) {
                    toolCall.toolName = func["name"].get<std::string>();
                }
                
                if (func.contains("arguments")) {
                    std::string argumentsStr = func["arguments"].get<std::string>();
                    
                    // Parse the arguments JSON string
                    try {
                        json argumentsJson = json::parse(argumentsStr);
                        for (const auto& [key, value] : argumentsJson.items()) {
                            toolCall.parameters[key] = value.get<std::string>();
                        }
                    } catch (const json::exception& e) {
                        std::cout << "DEBUG: Failed to parse arguments JSON: " << e.what() << std::endl;
                    }
                }
            }
            
            std::cout << "DEBUG: Parsed JSON tool call - name: " << toolCall.toolName << std::endl;
            for (const auto& [key, value] : toolCall.parameters) {
                std::cout << "DEBUG: Parameter: " << key << " = " << value << std::endl;
            }
            
            return toolCall;
            
        } catch (const json::exception& e) {
            std::cout << "DEBUG: Failed to parse tool call JSON: " << e.what() << std::endl;
            // Fall through to legacy parsing
        }
    }
    
    // Legacy parsing for old format (TOOL_CALL:toolName:param=value)
    // Find the first colon after TOOL_CALL: or TOOL:
    std::string::size_type firstColon = toolCallStr.find(':');
    if (firstColon == std::string::npos) {
        return toolCall;
    }
    
    // Find the second colon (after the tool name)
    std::string::size_type secondColon = toolCallStr.find(':', firstColon + 1);
    if (secondColon == std::string::npos) {
        return toolCall;
    }
    
    // Extract tool name
    std::string toolName = toolCallStr.substr(firstColon + 1, secondColon - firstColon - 1);
    // Trim whitespace from tool name
    toolName.erase(0, toolName.find_first_not_of(" \t"));
    toolName.erase(toolName.find_last_not_of(" \t") + 1);
    toolCall.toolName = toolName;
    
    // Extract parameters string (everything after the second colon)
    std::string paramsStr = toolCallStr.substr(secondColon + 1);
    // Trim whitespace from params string
    paramsStr.erase(0, paramsStr.find_first_not_of(" \t"));
    paramsStr.erase(paramsStr.find_last_not_of(" \t") + 1);
    
    std::cout << "DEBUG: Tool name: " << toolCall.toolName << std::endl;
    std::cout << "DEBUG: Params string length: " << paramsStr.length() << std::endl;
    std::cout << "DEBUG: Params string: [" << paramsStr << "]" << std::endl;
    
    // Special handling for writeFile tool with content parameter
    if (toolCall.toolName == "writeFile") {
        // For writeFile, we expect: path=value:content=multiline_content
        size_t pathStart = paramsStr.find("path=");
        size_t contentStart = paramsStr.find(":content=");
        
        if (pathStart != std::string::npos && contentStart != std::string::npos) {
            // Extract path parameter
            size_t pathValueStart = pathStart + 5; // "path=" is 5 characters
            size_t pathValueEnd = contentStart;
            std::string pathValue = paramsStr.substr(pathValueStart, pathValueEnd - pathValueStart);
            // Trim whitespace
            pathValue.erase(0, pathValue.find_first_not_of(" \t"));
            pathValue.erase(pathValue.find_last_not_of(" \t") + 1);
            toolCall.parameters["path"] = pathValue;
            
            // Extract content parameter (everything after ":content=")
            size_t contentValueStart = contentStart + 9; // ":content=" is 9 characters
            std::string contentValue = paramsStr.substr(contentValueStart);
            // Trim whitespace
            contentValue.erase(0, contentValue.find_first_not_of(" \t"));
            contentValue.erase(contentValue.find_last_not_of(" \t") + 1);
            toolCall.parameters["content"] = contentValue;
            
            std::cout << "DEBUG: Parsed path = [" << pathValue << "]" << std::endl;
            std::cout << "DEBUG: Parsed content length = " << contentValue.length() << std::endl;
            std::cout << "DEBUG: Parsed content = [" << contentValue << "]" << std::endl;
        } else {
            std::cout << "DEBUG: Could not find path= or :content= in writeFile parameters" << std::endl;
        }
    } else if (toolCall.toolName == "editFile") {
        // For editFile, we expect: target_file=value:instructions=value:code_edit=multiline_content
        size_t targetFileStart = paramsStr.find("target_file=");
        size_t instructionsStart = paramsStr.find(":instructions=");
        size_t codeEditStart = paramsStr.find(":code_edit=");
        
        if (targetFileStart != std::string::npos && instructionsStart != std::string::npos && codeEditStart != std::string::npos) {
            // Extract target_file parameter
            size_t targetFileValueStart = targetFileStart + 12; // "target_file=" is 12 characters
            size_t targetFileValueEnd = instructionsStart;
            std::string targetFileValue = paramsStr.substr(targetFileValueStart, targetFileValueEnd - targetFileValueStart);
            // Trim whitespace
            targetFileValue.erase(0, targetFileValue.find_first_not_of(" \t"));
            targetFileValue.erase(targetFileValue.find_last_not_of(" \t") + 1);
            toolCall.parameters["target_file"] = targetFileValue;
            
            // Extract instructions parameter
            size_t instructionsValueStart = instructionsStart + 13; // ":instructions=" is 13 characters
            size_t instructionsValueEnd = codeEditStart;
            std::string instructionsValue = paramsStr.substr(instructionsValueStart, instructionsValueEnd - instructionsValueStart);
            // Trim whitespace
            instructionsValue.erase(0, instructionsValue.find_first_not_of(" \t"));
            instructionsValue.erase(instructionsValue.find_last_not_of(" \t") + 1);
            toolCall.parameters["instructions"] = instructionsValue;
            
            // Extract code_edit parameter (everything after ":code_edit=")
            size_t codeEditValueStart = codeEditStart + 10; // ":code_edit=" is 10 characters
            std::string codeEditValue = paramsStr.substr(codeEditValueStart);
            // Trim whitespace
            codeEditValue.erase(0, codeEditValue.find_first_not_of(" \t"));
            codeEditValue.erase(codeEditValue.find_last_not_of(" \t") + 1);
            toolCall.parameters["code_edit"] = codeEditValue;
            
            std::cout << "DEBUG: Parsed target_file = [" << targetFileValue << "]" << std::endl;
            std::cout << "DEBUG: Parsed instructions = [" << instructionsValue << "]" << std::endl;
            std::cout << "DEBUG: Parsed code_edit length = " << codeEditValue.length() << std::endl;
        } else {
            std::cout << "DEBUG: Could not find target_file=, :instructions=, or :code_edit= in editFile parameters" << std::endl;
        }
    } else {
        // For other tools, use the original parsing logic
        size_t pos = 0;
        while (pos < paramsStr.length()) {
            size_t equalPos = paramsStr.find('=', pos);
            if (equalPos == std::string::npos) break;
            
            std::string paramName = paramsStr.substr(pos, equalPos - pos);
            // Trim whitespace from parameter name
            paramName.erase(0, paramName.find_first_not_of(" \t"));
            paramName.erase(paramName.find_last_not_of(" \t") + 1);
            
            // Find the end of this parameter value
            size_t valueStart = equalPos + 1;
            size_t valueEnd = paramsStr.find(':', valueStart);
            if (valueEnd == std::string::npos) {
                valueEnd = paramsStr.length();
            }
            
            std::string paramValue = paramsStr.substr(valueStart, valueEnd - valueStart);
            // Trim whitespace from parameter value
            paramValue.erase(0, paramValue.find_first_not_of(" \t"));
            paramValue.erase(paramValue.find_last_not_of(" \t") + 1);
            
            toolCall.parameters[paramName] = paramValue;
            
            std::cout << "DEBUG: Parsed parameter: " << paramName << " = [" << paramValue << "]" << std::endl;
            
            pos = valueEnd;
            if (pos < paramsStr.length() && paramsStr[pos] == ':') {
                pos++; // Skip the colon
            }
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
    } else if (toolCall.toolName == "readFile") {
        auto it = toolCall.parameters.find("path");
        if (it != toolCall.parameters.end()) {
            FileSystemServer fsServer;
            std::string content = fsServer.readFile(it->second);
            
            if (!content.empty()) {
                return content;
            } else {
                return "Error: File '" + it->second + "' does not exist or is empty.";
            }
        } else {
            return "Error: Missing 'path' parameter for readFile tool";
        }
    } else if (toolCall.toolName == "writeFile") {
        std::cout << "DEBUG: Executing writeFile tool" << std::endl;
        std::cout << "DEBUG: Number of parameters: " << toolCall.parameters.size() << std::endl;
        for (const auto& param : toolCall.parameters) {
            std::cout << "DEBUG: Parameter: " << param.first << " = " << param.second << std::endl;
        }
        
        auto it = toolCall.parameters.find("path");
        if (it != toolCall.parameters.end()) {
            auto itContent = toolCall.parameters.find("content");
            if (itContent != toolCall.parameters.end()) {
                FileSystemServer fsServer;
                bool success = fsServer.writeFile(it->second, itContent->second);
                
                if (success) {
                    return "Success: File '" + it->second + "' written successfully.";
                } else {
                    return "Error: Failed to write to file '" + it->second + "'. The file may not exist or there may be permission issues.";
                }
            } else {
                return "Error: Missing 'content' parameter for writeFile tool";
            }
        } else {
            return "Error: Missing 'path' parameter for writeFile tool";
        }
    } else if (toolCall.toolName == "editFile") {
        auto it = toolCall.parameters.find("target_file");
        auto itInstructions = toolCall.parameters.find("instructions");
        auto itCodeEdit = toolCall.parameters.find("code_edit");
        if (it != toolCall.parameters.end() && itInstructions != toolCall.parameters.end() && itCodeEdit != toolCall.parameters.end()) {
            FileSystemServer fsServer;
            std::string result = fsServer.editFile(it->second, itInstructions->second, itCodeEdit->second);
            return result;
        } else {
            return "Error: Missing 'target_file', 'instructions', or 'code_edit' parameter for editFile tool";
        }
    } else if (toolCall.toolName == "executeCommand") {
        auto it = toolCall.parameters.find("command");
        if (it != toolCall.parameters.end()) {
            TerminalServer terminalServer;
            std::string result = terminalServer.executeCommand(it->second);
            return result;
        } else {
            return "Error: Missing 'command' parameter for executeCommand tool";
        }
    } else if (toolCall.toolName == "executeCommandWithErrorCapture") {
        auto it = toolCall.parameters.find("command");
        if (it != toolCall.parameters.end()) {
            TerminalServer terminalServer;
            std::string result = terminalServer.executeCommandWithErrorCapture(it->second);
            return result;
        } else {
            return "Error: Missing 'command' parameter for executeCommandWithErrorCapture tool";
        }
    } else if (toolCall.toolName == "executeCommandInDirectory") {
        auto itCommand = toolCall.parameters.find("command");
        auto itDir = toolCall.parameters.find("workingDirectory");
        if (itCommand != toolCall.parameters.end() && itDir != toolCall.parameters.end()) {
            TerminalServer terminalServer;
            std::string result = terminalServer.executeCommandInDirectory(itCommand->second, itDir->second);
            return result;
        } else {
            return "Error: Missing 'command' or 'workingDirectory' parameter for executeCommandInDirectory tool";
        }
    } else if (toolCall.toolName == "commandExists") {
        auto it = toolCall.parameters.find("command");
        if (it != toolCall.parameters.end()) {
            TerminalServer terminalServer;
            bool exists = terminalServer.commandExists(it->second);
            return exists ? "Command '" + it->second + "' exists in PATH." : "Command '" + it->second + "' does not exist in PATH.";
        } else {
            return "Error: Missing 'command' parameter for commandExists tool";
        }
    } else if (toolCall.toolName == "getCurrentWorkingDirectory") {
        TerminalServer terminalServer;
        std::string cwd = terminalServer.getCurrentWorkingDirectory();
        return cwd;
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