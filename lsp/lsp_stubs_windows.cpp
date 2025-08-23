// Windows stub implementations for LSP functionality
// Most LSP features are disabled on Windows due to Unix-specific process management
// requirements However, basic Python support via pyright is implemented using
// Windows-native LSP manager

#include "../editor/editor.h"
#include "../editor/editor_scroll.h"
#include "../editor/editor_types.h"
#include "../files/files.h"
#include "../globals.h"
#include "lsp.h"
#include "lsp_autocomplete.h"
#include "lsp_goto_def.h"
#include "lsp_goto_ref.h"
#include "lsp_manager_windows.h"
#include "lsp_symbol_info.h"
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <windows.h>
#include <chrono>

// Global instances 
EditorLSP gEditorLSP;
LSPAutocomplete gLSPAutocomplete;
// Note: gLSPGotoDef, gLSPGotoRef, and gLSPSymbolInfo are now defined in their
// cross-platform files

// Static member definition
bool LSPAutocomplete::wasShowingLastFrame = false;

// Simple Windows LSP implementation
EditorLSP::EditorLSP() {}
EditorLSP::~EditorLSP() {}

bool EditorLSP::initialize(const std::string &workspacePath)
{
	return gLSPManagerWindows.initialize(workspacePath);
}

std::string EditorLSP::escapeJSON(const std::string &s) const 
{
	std::string escapedContent;
	for (char c : s)
	{
		switch (c)
		{
		case '"':
			escapedContent += "\\\"";
			break;
		case '\\':
			escapedContent += "\\\\";
			break;
		case '\n':
			escapedContent += "\\n";
			break;
		case '\r':
			escapedContent += "\\r";
			break;
		case '\t':
			escapedContent += "\\t";
			break;
		default:
			escapedContent += c;
			break;
		}
	}
	return escapedContent;
}

// didOpen and didChange methods are now handled by the main lsp.cpp implementation
// to avoid conflicts and bugs

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

	// Delegate to the Windows LSP manager for file type detection and initialization
	if (!gLSPManagerWindows.selectAdapterForFile(filePath))
	{
		return; // No adapter available for this file type
	}

	// Auto-initialize if needed
	if (!gLSPManagerWindows.isInitialized())
	{
		std::string workspacePath = filePath.substr(0, filePath.find_last_of("/\\"));
		if (!gLSPManagerWindows.initialize(workspacePath))
		{
			return; // Failed to initialize
		}
	}

	// Convert Windows path to URI
	std::string fileURI = "file:///" + filePath;
	for (char &c : fileURI)
	{
		if (c == '\\')
			c = '/';
	}

	std::string notification = std::string(R"({
		"jsonrpc": "2.0",
		"method": "textDocument/didOpen",
		"params": {
			"textDocument": {
				"uri": ")") + fileURI + R"(",
				"languageId": ")" + gLSPManagerWindows.getLanguageId(filePath) + R"(",
				"version": 1,
				"text": ")" + escapeJSON(content) + R"("
			}
		}
	})";

	if (gLSPManagerWindows.hasWorkingAdapter())
	{
		gLSPManagerWindows.sendRequest(notification);
		openedFiles.insert(filePath);
		std::cout << "\033[32mLSP:\033[0m didOpen notification sent for: " << filePath << std::endl;
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

	// Delegate to the Windows LSP manager for file type detection and initialization
	if (!gLSPManagerWindows.selectAdapterForFile(filePath))
	{
		return; // No adapter available for this file type
	}

	// Auto-initialize if needed
	if (!gLSPManagerWindows.isInitialized())
	{
		std::string workspacePath = filePath.substr(0, filePath.find_last_of("/\\"));
		if (!gLSPManagerWindows.initialize(workspacePath))
		{
			return; // Failed to initialize
		}
	}

	// Convert Windows path to URI
	std::string fileURI = "file:///" + filePath;
	for (char &c : fileURI)
	{
		if (c == '\\')
			c = '/';
	}

	std::string notification = std::string(R"({
		"jsonrpc": "2.0",
		"method": "textDocument/didChange",
		"params": {
			"textDocument": {
				"uri": ")") + fileURI + R"(",
				"version": )" + std::to_string(version) + R"(
			},
			"contentChanges": [{
				"text": ")" + escapeJSON(editor_state.fileContent) + R"("
			}]
		}
	})";

	if (gLSPManagerWindows.hasWorkingAdapter())
	{
		gLSPManagerWindows.sendRequest(notification);
		std::cout << "\033[36mLSP:\033[0m didChange notification sent (v" << version << ") for: " << filePath << std::endl;
	}
}

// LSPAutocomplete stub implementation
LSPAutocomplete::LSPAutocomplete() {}
LSPAutocomplete::~LSPAutocomplete() {}

void LSPAutocomplete::requestCompletion(const std::string &filePath,
										int line,
										int character)
{
	// Stub - no action on Windows
}

void LSPAutocomplete::renderCompletions()
{
	// Stub - no action on Windows
}

void LSPAutocomplete::processPendingResponses()
{
	// Stub - no action on Windows
}

bool LSPAutocomplete::shouldRender() { return false; }

bool LSPAutocomplete::handleInputAndCheckClose() { return true; }

void LSPAutocomplete::calculateWindowGeometry(ImVec2 &outWindowSize, ImVec2 &outSafePos)
{
	// Stub - no action on Windows
}

void LSPAutocomplete::applyStyling()
{
	// Stub - no action on Windows
}

void LSPAutocomplete::renderCompletionListItems()
{
	// Stub - no action on Windows
}

bool LSPAutocomplete::handleClickOutside() { return false; }

void LSPAutocomplete::finalizeRenderState()
{
	// Stub - no action on Windows
}

void LSPAutocomplete::resetPopupPosition()
{
	// Stub - no action on Windows
}

std::string LSPAutocomplete::formCompletionRequest(int requestId,
												   const std::string &filePath,
												   int line,
												   int character)
{
	return "";
}

bool LSPAutocomplete::processResponse(const std::string &response, int requestId)
{
	return false;
}

void LSPAutocomplete::parseCompletionResult(const json &result,
											int requestLine,
											int requestCharacter)
{
	// Stub - no action on Windows
}

void LSPAutocomplete::updatePopupPosition()
{
	// Stub - no action on Windows
}

void LSPAutocomplete::workerFunction()
{
	// Stub - no action on Windows
}

void LSPAutocomplete::insertText(
	int row_start, int col_start, int row_end, int col_end, std::string text)
{
	// Stub - no action on Windows
}

// LSPGotoDef implementation now in main lsp_goto_def.cpp for cross-platform support

// LSPGotoRef implementation now in main lsp_goto_ref.cpp for cross-platform support

// LSPSymbolInfo implementation now in main lsp_symbol_info.cpp for cross-platform support