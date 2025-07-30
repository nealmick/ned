#pragma once

#include <string>
#include <vector>

namespace MCP {

class FileSystemServer
{
  public:
	FileSystemServer();
	~FileSystemServer() = default;

	// Main method to list files in a directory
	std::vector<std::string> listFiles(const std::string &path);

	// Method to create a new file
	bool createFile(const std::string &path);

	// Method to create a new folder
	bool createFolder(const std::string &path);

	// Method to read file contents with sensible truncation
	std::string readFile(const std::string &path);

	// Edit file using Morph's fast apply API
	std::string editFile(const std::string &target_file,
						 const std::string &instructions,
						 const std::string &code_edit);
};

} // namespace MCP