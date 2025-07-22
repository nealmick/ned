#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace MCP {

struct ToolParameter
{
	std::string name;
	std::string type;
	std::string description;
	bool required;
};

struct ToolDefinition
{
	std::string name;
	std::string description;
	std::vector<ToolParameter> parameters;
};

struct ToolCall
{
	std::string toolName;
	std::unordered_map<std::string, std::string> parameters;
};

class Manager
{
  public:
	Manager();
	~Manager() = default;

	void registerTool(const ToolDefinition &toolDef);
	std::vector<ToolDefinition> getToolDefinitions() const;

	// Get tool definition by name
	const ToolDefinition *getToolDefinition(const std::string &name) const;

	// Execute a tool call and return the result
	std::string
	executeToolCall(const std::string &toolName,
					const std::unordered_map<std::string, std::string> &parameters);

  private:
	std::vector<ToolDefinition> toolDefinitions;
};

} // namespace MCP