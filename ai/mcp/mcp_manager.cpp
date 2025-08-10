#include "mcp_manager.h"
#include "../lib/json.hpp"
#include "mcp_file_system.h"
#include "mcp_terminal.h"
#include <iostream>
#include <regex>
#include <sstream>

using json = nlohmann::json;

namespace MCP {

Manager::Manager()
{
	// Register default tools
	ToolDefinition listFilesTool;
	listFilesTool.name = "listFiles";
	listFilesTool.description = "List files in a directory. Parameter: path "
								"(required) - the directory path to list.";

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
	createFileTool.description = "Create a new empty file. Parameter: path "
								 "(required) - the file path to create.";

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
	createFolderTool.description = "Create a new folder. Parameter: path "
								   "(required) - the folder path to create.";

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
	readFileTool.description = "Read the contents of a file. Parameter: path "
							   "(required) - the file path to read.";

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
	editFileTool.description =
		"Edit an existing file. Parameters: target_file (required) - file "
		"path, instructions "
		"(required) - what to do, code_edit (required) - the code changes.";

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
	executeCommandTool.description = "Run a terminal command. Parameter: "
									 "command (required) - the command to run.";

	ToolParameter commandParam;
	commandParam.name = "command";
	commandParam.type = "string";
	commandParam.description = "The command to run";
	commandParam.required = true;

	executeCommandTool.parameters.push_back(commandParam);
	registerTool(executeCommandTool);
}

void Manager::registerTool(const ToolDefinition &toolDef)
{
	toolDefinitions.push_back(toolDef);
}

std::vector<ToolDefinition> Manager::getToolDefinitions() const
{
	return toolDefinitions;
}

const ToolDefinition *Manager::getToolDefinition(const std::string &name) const
{
	for (const auto &tool : toolDefinitions)
	{
		if (tool.name == name)
		{
			return &tool;
		}
	}
	return nullptr;
}

std::string
Manager::executeToolCall(const std::string &toolName,
						 const std::unordered_map<std::string, std::string> &parameters)
{
	// Use stack allocation to avoid static destruction order issues
	FileSystemServer fileSystemServer;
	TerminalServer terminalServer;

	std::cout << "=== EXECUTING TOOL CALL ===" << std::endl;
	std::cout << "Tool: " << toolName << std::endl;
	std::cout << "Parameters:" << std::endl;
	for (const auto &param : parameters)
	{
		std::cout << "  " << param.first << ": " << param.second << std::endl;
	}
	std::cout << "==========================" << std::endl;

	try
	{
		if (toolName == "listFiles")
		{
			auto it = parameters.find("path");
			if (it != parameters.end())
			{
				auto files = fileSystemServer.listFiles(it->second);
				std::string result = "Files in " + it->second + ":\n";
				for (const auto &file : files)
				{
					result += file + "\n";
				}
				return result;
			}
		} else if (toolName == "createFile")
		{
			auto it = parameters.find("path");
			if (it != parameters.end())
			{
				bool success = fileSystemServer.createFile(it->second);
				std::string result = success ? "File created successfully: " + it->second
											 : "Failed to create file: " + it->second;
				return result;
			}
		} else if (toolName == "createFolder")
		{
			auto it = parameters.find("path");
			if (it != parameters.end())
			{
				bool success = fileSystemServer.createFolder(it->second);
				std::string result = success
										 ? "Folder created successfully: " + it->second
										 : "Failed to create folder: " + it->second;
				return result;
			}
		} else if (toolName == "readFile")
		{
			auto it = parameters.find("path");
			if (it != parameters.end())
			{
				std::string content = fileSystemServer.readFile(it->second);
				return content;
			}
		} else if (toolName == "editFile")
		{
			auto targetFileIt = parameters.find("target_file");
			auto instructionsIt = parameters.find("instructions");
			auto codeEditIt = parameters.find("code_edit");

			if (targetFileIt != parameters.end() && instructionsIt != parameters.end() &&
				codeEditIt != parameters.end())
			{
				std::string result = fileSystemServer.editFile(targetFileIt->second,
															   instructionsIt->second,
															   codeEditIt->second);
				return result;
			}
		} else if (toolName == "executeCommand")
		{
			auto it = parameters.find("command");
			if (it != parameters.end())
			{
				std::string output = terminalServer.executeCommand(it->second);
				return output;
			}
		}

		std::string error = "Unknown tool: " + toolName;
		return error;

	} catch (const std::exception &e)
	{
		std::string error = "Exception during tool execution: " + std::string(e.what());
		return error;
	}
}

} // namespace MCP

MCP::Manager gMCPManager;