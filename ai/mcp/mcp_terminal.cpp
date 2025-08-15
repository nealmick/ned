#include "mcp_terminal.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define getcwd _getcwd
#else
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace MCP {

TerminalServer::TerminalServer() {}

std::string TerminalServer::executeCommand(const std::string &command)
{
	try
	{
		// Use popen to execute the command and capture output
		std::array<char, 128> buffer;
		std::string result;

		// Execute command and capture stdout
#ifndef PLATFORM_WINDOWS
		std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
		if (!pipe)
		{
			return "ERROR: Failed to execute command '" + command + "'";
		}

		// Read the output
		while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
		{
			result += buffer.data();
		}

		// Get the exit status
		int status = pclose(pipe.release());

		// Add exit status information
		if (status != 0)
		{
			result += "\n[Command exited with status: " + std::to_string(status) + "]";
		}

		return result;
#else
		// Windows doesn't support popen/pclose easily, stub this functionality
		return "ERROR: MCP terminal commands not supported on Windows";
#endif

	} catch (const std::exception &e)
	{
		return "ERROR: " + std::string(e.what());
	} catch (...)
	{
		return "ERROR: Unknown error occurred while executing command";
	}
}

} // namespace MCP