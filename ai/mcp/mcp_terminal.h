#pragma once

#include <string>
#include <vector>

namespace MCP {

class TerminalServer
{
  public:
	TerminalServer();
	~TerminalServer() = default;

	// Main method to execute a terminal command and return its output
	std::string executeCommand(const std::string &command);
};

} // namespace MCP