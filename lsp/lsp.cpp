#include "lsp.h"
#include "../editor/editor.h"
#include <chrono>
#include <cstdio>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

#ifdef PLATFORM_WINDOWS
#include "lsp_manager_windows.h"
#define CURRENT_LSP_MANAGER gLSPManagerWindows
#else
#include "lsp_manager.h"
#define CURRENT_LSP_MANAGER gLSPManager
#endif

// Global instance
EditorLSP gEditorLSP;

EditorLSP::EditorLSP() : currentRequestId(1000) {}

EditorLSP::~EditorLSP() = default;

bool EditorLSP::initialize(const std::string &workspacePath)
{
	std::cout << "\033[35mLSP:\033[0m Initializing with workspace path: " << workspacePath
			  << std::endl;

	// Delegate to the platform-specific LSP manager
	return CURRENT_LSP_MANAGER.initialize(workspacePath);
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
				char buf[16];
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
	// Aggressive debouncing to prevent multiple didOpen calls
	static auto lastDidOpenTime = std::chrono::steady_clock::now();
	static std::set<std::string> openedFiles;
	auto currentTime = std::chrono::steady_clock::now();
	auto timeSinceLastDidOpen = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastDidOpenTime);
	
	// Check if file was already opened
	if (openedFiles.find(filePath) != openedFiles.end())
	{
		std::cout << "\033[33mLSP:\033[0m File already opened, ignoring: " << filePath << std::endl;
		return;
	}
	
	// General debouncing for didOpen calls
	if (timeSinceLastDidOpen.count() < 100) // 100ms between any didOpen calls
	{
		std::cout << "\033[33mLSP:\033[0m Ignoring rapid didOpen call\n";
		return;
	}
	
	lastDidOpenTime = currentTime;

	// Select the appropriate adapter for this file
	if (!CURRENT_LSP_MANAGER.selectAdapterForFile(filePath))
	{
		return;
	}

	// If the selected adapter is not initialized, initialize it with the
	// current file's directory
	if (!CURRENT_LSP_MANAGER.isInitialized())
	{
		// Extract workspace path from file path (use directory containing the
		// file)
		std::string workspacePath = filePath.substr(0, filePath.find_last_of("/\\"));
		std::cout << "\033[35mLSP:\033[0m Auto-initializing with workspace: "
				  << workspacePath << std::endl;

		try
		{
			if (!CURRENT_LSP_MANAGER.initialize(workspacePath))
			{
				std::cout << "\033[31mLSP:\033[0m Failed to initialize LSP for "
						  << filePath << std::endl;
				return;
			}
		} catch (const std::exception &e)
		{
			std::cout << "\033[31mLSP:\033[0m Exception during LSP initialization: "
					  << e.what() << std::endl;
			std::cout
				<< "\033[33mLSP:\033[0m LSP support will be disabled for this session"
				<< std::endl;
			return;
		} catch (...)
		{
			std::cout << "\033[31mLSP:\033[0m Unknown exception during LSP initialization"
					  << std::endl;
			std::cout
				<< "\033[33mLSP:\033[0m LSP support will be disabled for this session"
				<< std::endl;
			return;
		}
	}

#ifdef PLATFORM_WINDOWS
	// Windows path needs file:/// prefix
	std::string fileURI = "file:///" + filePath;
	for (char &c : fileURI)
	{
		if (c == '\\')
			c = '/';
	}
#else
	std::string fileURI = "file://" + filePath;
#endif

	std::string notification = std::string(R"({
            "jsonrpc": "2.0",
            "method": "textDocument/didOpen",
            "params": {
                "textDocument": {
                    "uri": ")") +
							   fileURI + R"(",
                    "languageId": ")" +
							   CURRENT_LSP_MANAGER.getLanguageId(filePath) + R"(",
                    "version": 1,
                    "text": ")" +
							   escapeJSON(content) +
							   R"("
                }
            }
        })";

	// Only send request if we have a working adapter
	if (CURRENT_LSP_MANAGER.hasWorkingAdapter())
	{
		CURRENT_LSP_MANAGER.sendRequest(notification);
		openedFiles.insert(filePath);
		std::cout << "\033[32mLSP:\033[0m didOpen notification sent for: " << filePath << std::endl;
	} else
	{
		std::cout
			<< "\033[33mLSP:\033[0m Skipping LSP request - no working adapter available"
			<< std::endl;
	}
}

void EditorLSP::didChange(const std::string &filePath, int version)
{
	// Debouncing for didChange calls
	static auto lastDidChangeTime = std::chrono::steady_clock::now();
	auto currentTime = std::chrono::steady_clock::now();
	auto timeSinceLastDidChange = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastDidChangeTime);
	
	// More aggressive debouncing for didChange since it can be called frequently during typing
	if (timeSinceLastDidChange.count() < 200) // 200ms between didChange calls
	{
		std::cout << "\033[33mLSP:\033[0m Ignoring rapid didChange call\n";
		return;
	}
	
	lastDidChangeTime = currentTime;

	// Select the appropriate adapter for this file
	if (!CURRENT_LSP_MANAGER.selectAdapterForFile(filePath))
	{
		return;
	}

	// Auto-initialize if needed
	if (!CURRENT_LSP_MANAGER.isInitialized())
	{
		std::string workspacePath = filePath.substr(0, filePath.find_last_of("/\\"));
		std::cout << "\033[35mLSP:\033[0m Auto-initializing with workspace: "
				  << workspacePath << std::endl;

		try
		{
			if (!CURRENT_LSP_MANAGER.initialize(workspacePath))
			{
				std::cout << "\033[31mLSP:\033[0m Failed to initialize LSP for "
						  << filePath << std::endl;
				return;
			}
		} catch (const std::exception &e)
		{
			std::cout << "\033[31mLSP:\033[0m Exception during LSP initialization: "
					  << e.what() << std::endl;
			std::cout
				<< "\033[33mLSP:\033[0m LSP support will be disabled for this session"
				<< std::endl;
			return;
		} catch (...)
		{
			std::cout << "\033[31mLSP:\033[0m Unknown exception during LSP initialization"
					  << std::endl;
			std::cout
				<< "\033[33mLSP:\033[0m LSP support will be disabled for this session"
				<< std::endl;
			return;
		}
	}

#ifdef PLATFORM_WINDOWS
	// Windows path needs file:/// prefix
	std::string fileURI = "file:///" + filePath;
	for (char &c : fileURI)
	{
		if (c == '\\')
			c = '/';
	}
#else
	std::string fileURI = "file://" + filePath;
#endif

	std::string notification = std::string(R"({
        "jsonrpc": "2.0",
        "method": "textDocument/didChange",
        "params": {
            "textDocument": {
                "uri": ")") +
							   fileURI + R"(",
                "version": )" + std::to_string(version) + R"(
            },
            "contentChanges": [
                {
                    "text": ")" +
							   escapeJSON(editor_state.fileContent) + R"("
                }
            ]
        }
    })";

	// Only send request if we have a working adapter
	if (CURRENT_LSP_MANAGER.hasWorkingAdapter())
	{
		if (CURRENT_LSP_MANAGER.sendRequest(notification))
		{
			std::cout << "\033[36mLSP:\033[0m didChange notification sent (v" << version << ") for: " << filePath << std::endl;
		} else
		{
			std::cout << "\033[31mLSP:\033[0m Failed to send didChange notification\n";
		}
	} else
	{
		std::cout
			<< "\033[33mLSP:\033[0m Skipping LSP request - no working adapter available"
			<< std::endl;
	}
}