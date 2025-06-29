#include "mcp_manager.h"
#include "mcp_file_system.h"
#include "mcp_terminal.h"
#include "../lib/json.hpp"
#include <sstream>
#include <regex>
#include <iostream>

using json = nlohmann::json;

namespace MCP {

// SYSTEM PROMPT FOR AGENT TOOL USE:
//
// You are an AI agent that can ONLY interact with the world using the tools listed below.
//
// RULES:
// 1. Only use the tools listed. Do not invent or mimic tool calls or results.
// 2. NEVER output anything that looks like a tool call result unless it is the actual result from the tool.
// 3. Every tool call MUST include all required parameters. If you do not have all required parameters, do not call the tool.
// 4. Tool call formats:
//    - Legacy: TOOL_CALL:toolName:param1=value:param2=value
//    - JSON:   TOOL_CALL:{"function":{"name":"toolName","arguments":"{\"param1\":\"value\",\"param2\":\"value\"}"}}
// 5. Do NOT output plans, explanations, or step-by-step reasoning. Only output the tool call needed to accomplish the user's request.
// 6. Be concise. Do not add extra text, apologies, or commentary.
// 7. If you need to perform multiple steps, do them one at a time, each as a separate tool call.
// 8. If you do not know what to do, ask for clarification.
//
// EXAMPLES:
// To read a file: TOOL_CALL:readFile:path=main.cpp
// To edit a file: TOOL_CALL:editFile:target_file=main.cpp:instructions=Add a comment:code_edit=// ... existing code ...\n// great success
// To run a command: TOOL_CALL:executeCommand:command=ls -la
//
// NEVER output anything that looks like a tool result unless it is the real result from the tool.
//
// END SYSTEM PROMPT

Manager::Manager() {
    // Register default tools
    ToolDefinition listFilesTool;
    listFilesTool.name = "listFiles";
    listFilesTool.description = "List files in a directory. Parameter: path (required) - the directory path to list.";
    
    ToolParameter pathParam;
    pathParam.name = "path";
    pathParam.type = "string";
    pathParam.description = "Directory path to list files from";
    pathParam.required = true;
    
    listFilesTool.parameters.push_back(pathParam);
    registerTool(listFilesTool);
    
    // Register createFile tool
    ToolDefinition createFileTool;
    createFileTool.name = "createFile";
    createFileTool.description = "Create a new empty file. Parameter: path (required) - the file path to create.";
    
    ToolParameter createFilePathParam;
    createFilePathParam.name = "path";
    createFilePathParam.type = "string";
    createFilePathParam.description = "File path to create";
    createFilePathParam.required = true;
    
    createFileTool.parameters.push_back(createFilePathParam);
    registerTool(createFileTool);
    
    // Register createFolder tool
    ToolDefinition createFolderTool;
    createFolderTool.name = "createFolder";
    createFolderTool.description = "Create a new folder. Parameter: path (required) - the folder path to create.";
    
    ToolParameter createFolderPathParam;
    createFolderPathParam.name = "path";
    createFolderPathParam.type = "string";
    createFolderPathParam.description = "Folder path to create";
    createFolderPathParam.required = true;
    
    createFolderTool.parameters.push_back(createFolderPathParam);
    registerTool(createFolderTool);
    
    // Register readFile tool
    ToolDefinition readFileTool;
    readFileTool.name = "readFile";
    readFileTool.description = "Read the contents of a file. Parameter: path (required) - the file path to read.";
    
    ToolParameter readFilePathParam;
    readFilePathParam.name = "path";
    readFilePathParam.type = "string";
    readFilePathParam.description = "File path to read";
    readFilePathParam.required = true;
    
    readFileTool.parameters.push_back(readFilePathParam);
    registerTool(readFileTool);
    
    // Register editFile tool
    ToolDefinition editFileTool;
    editFileTool.name = "editFile";
    editFileTool.description = "Edit an existing file. Parameters: target_file (required) - file path, instructions (required) - what to do, code_edit (required) - the code changes.";
    
    ToolParameter editFilePathParam;
    editFilePathParam.name = "target_file";
    editFilePathParam.type = "string";
    editFilePathParam.description = "The file to edit";
    editFilePathParam.required = true;
    
    ToolParameter editFileInstructionsParam;
    editFileInstructionsParam.name = "instructions";
    editFileInstructionsParam.type = "string";
    editFileInstructionsParam.description = "What you are doing to the file";
    editFileInstructionsParam.required = true;
    
    ToolParameter editFileCodeEditParam;
    editFileCodeEditParam.name = "code_edit";
    editFileCodeEditParam.type = "string";
    editFileCodeEditParam.description = "The code changes to make";
    editFileCodeEditParam.required = true;
    
    editFileTool.parameters.push_back(editFilePathParam);
    editFileTool.parameters.push_back(editFileInstructionsParam);
    editFileTool.parameters.push_back(editFileCodeEditParam);
    registerTool(editFileTool);
    
    // Register terminal tools
    ToolDefinition executeCommandTool;
    executeCommandTool.name = "executeCommand";
    executeCommandTool.description = "Run a terminal command. Parameter: command (required) - the command to run.";
    
    ToolParameter commandParam;
    commandParam.name = "command";
    commandParam.type = "string";
    commandParam.description = "The command to run";
    commandParam.required = true;
    
    executeCommandTool.parameters.push_back(commandParam);
    registerTool(executeCommandTool);
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
    // Be more selective to avoid false positives from tool descriptions
    
    std::cout << "hasToolCalls: searching for tool calls in response" << std::endl;
    std::cout << "hasToolCalls: response length: " << response.length() << std::endl;
    
    // Look for actual tool call patterns, not descriptions
    bool found = false;
    
    // Check for JSON format tool calls
    if (response.find("TOOL_CALL:{\"function\":") != std::string::npos) {
        found = true;
        std::cout << "hasToolCalls: found JSON format tool call" << std::endl;
    }
    
    // Check for legacy format tool calls (but be more selective)
    if (response.find("TOOL_CALL:") != std::string::npos) {
        // Make sure it's not just a description by checking for actual tool names
        std::vector<std::string> validTools = {
            "listFiles", "createFile", "createFolder", "readFile", 
            "editFile", "executeCommand"
        };
        
        for (const auto& tool : validTools) {
            if (response.find("TOOL_CALL:" + tool + ":") != std::string::npos) {
                found = true;
                std::cout << "hasToolCalls: found legacy format tool call for " << tool << std::endl;
                break;
            }
        }
    }
    
    // Check for the old TOOL: format
    if (response.find("TOOL:") != std::string::npos) {
        found = true;
        std::cout << "hasToolCalls: found old TOOL: format" << std::endl;
    }
    
    // Check for the marker
    if (response.find("[TOOL_CALLS_DETECTED]") != std::string::npos) {
        found = true;
        std::cout << "hasToolCalls: found TOOL_CALLS_DETECTED marker" << std::endl;
    }
    
    std::cout << "hasToolCalls: found = " << (found ? "true" : "false") << std::endl;
    
    if (found) {
        std::cout << "hasToolCalls: found valid tool call pattern in response" << std::endl;
    } else {
        std::cout << "hasToolCalls: no valid tool call found in response" << std::endl;
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
    int processedCalls = 0;
    const int MAX_TOOL_CALLS = 10; // Prevent infinite loops
    
    while (pos < result.length() && processedCalls < MAX_TOOL_CALLS) {
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
        
        // Skip if this looks like a description rather than an actual tool call
        if (toolCallStr.find("Format:") != std::string::npos || 
            toolCallStr.find("Example") != std::string::npos ||
            toolCallStr.find("Required parameters:") != std::string::npos) {
            std::cout << "DEBUG: Skipping tool call that appears to be a description" << std::endl;
            pos = toolEnd;
            processedCalls++;
            continue;
        }
        
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
                "tool: " + toolCall.toolName + "\n"
                "Result: " + toolResult);
            
            processedCalls++;
            // Update position to continue searching
            pos = toolStart + 1;
        } else {
            // Skip invalid tool calls and move search position
            std::cout << "DEBUG: Invalid tool call detected, skipping" << std::endl;
            pos = toolEnd;
            processedCalls++; // Count invalid calls too to prevent infinite loops
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
        
        // Clean up the JSON string - remove any trailing characters that might break parsing
        size_t jsonEnd = jsonPart.find_last_of('}');
        if (jsonEnd != std::string::npos) {
            jsonPart = jsonPart.substr(0, jsonEnd + 1);
        }
        
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
                        std::cout << "DEBUG: Arguments string was: [" << argumentsStr << "]" << std::endl;
                        
                        // Try to extract parameters manually if JSON parsing fails
                        if (argumentsStr.find("path") != std::string::npos) {
                            size_t pathStart = argumentsStr.find("\"path\":\"");
                            if (pathStart != std::string::npos) {
                                pathStart += 8; // Skip "\"path\":\""
                                size_t pathEnd = argumentsStr.find("\"", pathStart);
                                if (pathEnd != std::string::npos) {
                                    std::string path = argumentsStr.substr(pathStart, pathEnd - pathStart);
                                    toolCall.parameters["path"] = path;
                                    std::cout << "DEBUG: Manually extracted path: " << path << std::endl;
                                }
                            }
                        }
                        
                        // Try to extract target_file parameter
                        if (argumentsStr.find("target_file") != std::string::npos) {
                            size_t targetFileStart = argumentsStr.find("\"target_file\":\"");
                            if (targetFileStart != std::string::npos) {
                                targetFileStart += 15; // Skip "\"target_file\":\""
                                size_t targetFileEnd = argumentsStr.find("\"", targetFileStart);
                                if (targetFileEnd != std::string::npos) {
                                    std::string targetFile = argumentsStr.substr(targetFileStart, targetFileEnd - targetFileStart);
                                    toolCall.parameters["target_file"] = targetFile;
                                    std::cout << "DEBUG: Manually extracted target_file: " << targetFile << std::endl;
                                }
                            }
                        }
                        
                        // Try to extract command parameter
                        if (argumentsStr.find("command") != std::string::npos) {
                            size_t commandStart = argumentsStr.find("\"command\":\"");
                            if (commandStart != std::string::npos) {
                                commandStart += 11; // Skip "\"command\":\""
                                size_t commandEnd = argumentsStr.find("\"", commandStart);
                                if (commandEnd != std::string::npos) {
                                    std::string command = argumentsStr.substr(commandStart, commandEnd - commandStart);
                                    toolCall.parameters["command"] = command;
                                    std::cout << "DEBUG: Manually extracted command: " << command << std::endl;
                                }
                            }
                        }
                        
                        // Try to extract instructions parameter
                        if (argumentsStr.find("instructions") != std::string::npos) {
                            size_t instructionsStart = argumentsStr.find("\"instructions\":\"");
                            if (instructionsStart != std::string::npos) {
                                instructionsStart += 16; // Skip "\"instructions\":\""
                                size_t instructionsEnd = argumentsStr.find("\"", instructionsStart);
                                if (instructionsEnd != std::string::npos) {
                                    std::string instructions = argumentsStr.substr(instructionsStart, instructionsEnd - instructionsStart);
                                    toolCall.parameters["instructions"] = instructions;
                                    std::cout << "DEBUG: Manually extracted instructions: " << instructions << std::endl;
                                }
                            }
                        }
                        
                        // Try to extract code_edit parameter
                        if (argumentsStr.find("code_edit") != std::string::npos) {
                            size_t codeEditStart = argumentsStr.find("\"code_edit\":\"");
                            if (codeEditStart != std::string::npos) {
                                codeEditStart += 13; // Skip "\"code_edit\":\""
                                size_t codeEditEnd = argumentsStr.find("\"", codeEditStart);
                                if (codeEditEnd != std::string::npos) {
                                    std::string codeEdit = argumentsStr.substr(codeEditStart, codeEditEnd - codeEditStart);
                                    toolCall.parameters["code_edit"] = codeEdit;
                                    std::cout << "DEBUG: Manually extracted code_edit: " << codeEdit << std::endl;
                                }
                            }
                        }
                        
                        // Try to extract content parameter
                        if (argumentsStr.find("content") != std::string::npos) {
                            size_t contentStart = argumentsStr.find("\"content\":\"");
                            if (contentStart != std::string::npos) {
                                contentStart += 12; // Skip "\"content\":\""
                                size_t contentEnd = argumentsStr.find("\"", contentStart);
                                if (contentEnd != std::string::npos) {
                                    std::string content = argumentsStr.substr(contentStart, contentEnd - contentStart);
                                    toolCall.parameters["content"] = content;
                                    std::cout << "DEBUG: Manually extracted content: " << content << std::endl;
                                }
                            }
                        }
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
            std::cout << "DEBUG: JSON string was: [" << jsonPart << "]" << std::endl;
            
            // Try to extract basic information even from malformed JSON
            if (jsonPart.find("\"name\":") != std::string::npos) {
                size_t nameStart = jsonPart.find("\"name\":\"");
                if (nameStart != std::string::npos) {
                    nameStart += 8; // Skip "\"name\":\""
                    size_t nameEnd = jsonPart.find("\"", nameStart);
                    if (nameEnd != std::string::npos) {
                        toolCall.toolName = jsonPart.substr(nameStart, nameEnd - nameStart);
                        std::cout << "DEBUG: Extracted tool name from malformed JSON: " << toolCall.toolName << std::endl;
                    }
                }
            }
            
            // Fall through to legacy parsing
        }
    }
    
    // Legacy parsing for old format (TOOL_CALL:toolName:param=value)
    // Find the first colon after TOOL_CALL: or TOOL:
    std::string::size_type firstColon = toolCallStr.find(':');
    if (firstColon == std::string::npos) {
        std::cout << "DEBUG: No first colon found in tool call string" << std::endl;
        return toolCall;
    }
    
    // Find the second colon (after the tool name)
    std::string::size_type secondColon = toolCallStr.find(':', firstColon + 1);
    if (secondColon == std::string::npos) {
        // This might be a tool call with no parameters
        std::string toolName = toolCallStr.substr(firstColon + 1);
        // Trim whitespace from tool name
        toolName.erase(0, toolName.find_first_not_of(" \t"));
        toolName.erase(toolName.find_last_not_of(" \t") + 1);
        toolCall.toolName = toolName;
        std::cout << "DEBUG: Tool call with no parameters - name: " << toolCall.toolName << std::endl;
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
    
    if (toolCall.toolName == "editFile") {
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
            std::cout << "DEBUG: Parsed code_edit = [" << codeEditValue << "]" << std::endl;
        } else {
            std::cout << "DEBUG: Could not find target_file=, :instructions=, or :code_edit= in editFile parameters" << std::endl;
            std::cout << "DEBUG: paramsStr = [" << paramsStr << "]" << std::endl;
            std::cout << "DEBUG: target_file found at: " << (targetFileStart != std::string::npos ? std::to_string(targetFileStart) : "NOT_FOUND") << std::endl;
            std::cout << "DEBUG: instructions found at: " << (instructionsStart != std::string::npos ? std::to_string(instructionsStart) : "NOT_FOUND") << std::endl;
            std::cout << "DEBUG: code_edit found at: " << (codeEditStart != std::string::npos ? std::to_string(codeEditStart) : "NOT_FOUND") << std::endl;
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
                return "Error: Failed to create file '" + it->second + "'.";
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
                return "Error: Failed to create folder '" + it->second + "'.";
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
    } else if (toolCall.toolName == "editFile") {
        auto it = toolCall.parameters.find("target_file");
        auto itInstructions = toolCall.parameters.find("instructions");
        auto itCodeEdit = toolCall.parameters.find("code_edit");
        if (it != toolCall.parameters.end() && itInstructions != toolCall.parameters.end() && itCodeEdit != toolCall.parameters.end()) {
            FileSystemServer fsServer;
            std::string result = fsServer.editFile(it->second, itInstructions->second, itCodeEdit->second);
            return result;
        } else {
            return "Error: Missing required parameters for editFile tool (need target_file, instructions, and code_edit)";
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