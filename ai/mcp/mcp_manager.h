#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace MCP {

struct ToolParameter {
    std::string name;
    std::string type;
    std::string description;
    bool required;
};

struct ToolDefinition {
    std::string name;
    std::string description;
    std::vector<ToolParameter> parameters;
    // The function to call for this tool (could be std::function<void(const std::unordered_map<std::string, std::string>&)>)
};

struct ToolCall {
    std::string toolName;
    std::unordered_map<std::string, std::string> parameters;
};

class Manager {
public:
    Manager();
    ~Manager() = default;

    void registerTool(const ToolDefinition& toolDef);
    std::vector<ToolDefinition> getToolDefinitions() const;
    std::string getToolDescriptionsJSON() const;

    // Parse LLM response for tool calls
    std::string processToolCalls(const std::string& llmResponse) const;
    
    // Check if response contains tool calls
    bool hasToolCalls(const std::string& response) const;

    // Route a tool call by name and parameters (parameters as string map for now)
    bool routeToolCall(const std::string& toolName, const std::unordered_map<std::string, std::string>& params);

    // Make executeToolCall public for modern tool calling
    std::string executeToolCall(const ToolCall& toolCall) const;

    // Parse a single tool call from string (public for debugging)
    ToolCall parseToolCall(const std::string& toolCallStr) const;

    // Get tool definition by name
    const ToolDefinition* getToolDefinition(const std::string& name) const;

private:
    std::vector<ToolDefinition> toolDefinitions;
    // Optionally, a map from tool name to function pointer for routing
};

} // namespace MCP 