#include "lsp.h"
#include "../editor/editor.h"
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

// Global instance
EditorLSP gEditorLSP;

EditorLSP::EditorLSP() : currentRequestId(1000) {}

EditorLSP::~EditorLSP() = default;

bool EditorLSP::initialize(const std::string &workspacePath)
{
	std::cout << "\033[35mLSP:\033[0m Initializing with workspace path: " << workspacePath
			  << std::endl;

	// Delegate to the LSP manager
	return gLSPManager.initialize(workspacePath);
}

std::string EditorLSP::escapeJSON(const std::string &s) const
{
	std::string out;
	out.reserve(s.length() * 2);
	for (char c : s)
	{
		switch (c)
		{
		case '"':
			out += "\\\"";
			break;
		case '\\':
			out += "\\\\";
			break;
		case '\b':
			out += "\\b";
			break;
		case '\f':
			out += "\\f";
			break;
		case '\n':
			out += "\\n";
			break;
		case '\r':
			out += "\\r";
			break;
		case '\t':
			out += "\\t";
			break;
		default:
			if ('\x00' <= c && c <= '\x1f')
			{
				char buf[8];
				snprintf(buf, sizeof buf, "\\u%04x", c);
				out += buf;
			} else
			{
				out += c;
			}
		}
	}
	return out;
}

void EditorLSP::didOpen(const std::string &filePath, const std::string &content)
{
	// Select the appropriate adapter for this file
	if (!gLSPManager.selectAdapterForFile(filePath))
	{
		std::cout << "\033[31mLSP:\033[0m No LSP adapter available for file: " << filePath
				  << std::endl;
		return;
	}

	// If the selected adapter is not initialized, initialize it with the
	// current file's directory
	if (!gLSPManager.isInitialized())
	{
		// Extract workspace path from file path (use directory containing the
		// file)
		std::string workspacePath = filePath.substr(0, filePath.find_last_of("/\\"));
		std::cout << "\033[35mLSP:\033[0m Auto-initializing with workspace: " << workspacePath
				  << std::endl;

		if (!gLSPManager.initialize(workspacePath))
		{
			std::cout << "\033[31mLSP:\033[0m Failed to initialize LSP for " << filePath
					  << std::endl;
			return;
		}
	}

	std::string notification = std::string(R"({
            "jsonrpc": "2.0",
            "method": "textDocument/didOpen",
            "params": {
                "textDocument": {
                    "uri": "file://)") +
							   filePath + R"(",
                    "languageId": ")" +
							   gLSPManager.getLanguageId(filePath) + R"(",
                    "version": 1,
                    "text": ")" +
							   escapeJSON(content) +
							   R"("
                }
            }
        })";

	gLSPManager.sendRequest(notification);
	std::cout << "\033[32mLSP:\033[0m didOpen notification sent successfully" << std::endl;
}

void EditorLSP::didChange(const std::string &filePath, int version)
{
	// Select the appropriate adapter for this file
	if (!gLSPManager.selectAdapterForFile(filePath))
	{
		std::cout << "\033[31mLSP:\033[0m No LSP adapter available for file: " << filePath
				  << std::endl;
		return;
	}

	// Auto-initialize if needed
	if (!gLSPManager.isInitialized())
	{
		std::string workspacePath = filePath.substr(0, filePath.find_last_of("/\\"));
		std::cout << "\033[35mLSP:\033[0m Auto-initializing with workspace: " << workspacePath
				  << std::endl;

		if (!gLSPManager.initialize(workspacePath))
		{
			std::cout << "\033[31mLSP:\033[0m Failed to initialize LSP for " << filePath
					  << std::endl;
			return;
		}
	}

	std::string notification = std::string(R"({
        "jsonrpc": "2.0",
        "method": "textDocument/didChange",
        "params": {
            "textDocument": {
                "uri": "file://)") +
							   filePath + R"(",
                "version": )" + std::to_string(version) +
							   R"(
            },
            "contentChanges": [
                {
                    "text": ")" +
							   escapeJSON(editor_state.fileContent) + R"("
                }
            ]
        }
    })";

	if (gLSPManager.sendRequest(notification))
	{
		// std::cout << "\033[32mLSP:\033[0m didChange notification sent successfully (v" << version
		// << ")\n";
	} else
	{
		// std::cout << "\033[31mLSP:\033[0m Failed to send didChange notification\n";
	}
}