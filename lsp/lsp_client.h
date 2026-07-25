#pragma once

#ifndef NED_ENABLE_LSP
#define NED_ENABLE_LSP 1
#endif

#include <string>
#include <vector>

class EditorApi;
class FileExplorer;
class Settings;

// Structure to hold language server information (used by settings UI when ON)
struct LanguageServerInfo
{
	std::string language;
	std::vector<std::string> fileExtensions;
	std::vector<std::string> serverPaths;
	std::vector<std::string> serverArgs;
};

#if NED_ENABLE_LSP

#include <functional>
#include <future>
#include <map>
#include <memory>
#include <thread>
#include <unordered_set>

#include "lsp_dashboard.h"
#include "lsp_goto_def.h"
#include "lsp_goto_ref.h"
#include "lsp_symbol_info.h"
#include "lsp_uri_options.h"

// Forward declarations
namespace lsp {

class Connection;
class MessageHandler;
class Process;

namespace io {

class Stream;

}
} // namespace lsp

class LSPClient
{
  public:
	LSPClient(EditorApi &api, FileExplorer &fileExplorer, Settings &settings);
	~LSPClient();

	LSPDashboard dashboard;
	LSPGotoDef gotoDef;
	LSPGotoRef gotoRef;
	LSPSymbolInfo symbolInfo;
	// Shared URI-options picker
	LSPUriOptions uriOptions;

	// Core LSP functionality
	void setWorkspace(const std::string &workspacePath);
	bool init(const std::string &filePath);
	void shutdown();
	bool isInitialized() const { return initialized; }
	std::string getCurrentLanguage() const { return currentLanguage; }

	// Language server information access
	const std::vector<LanguageServerInfo> &getLanguageServers() const
	{
		return languageServers;
	}
	std::vector<std::string> getSupportedLanguages() const;

	// Document management (version from EditorState; languageId is file extension id).
	// Open/close are balanced per path (normalized). Re-open of an already-open path
	// becomes didChange; didEdit/didClose no-op if the path is not tracked open.
	void didOpen(const std::string &filePath,
				 const std::string &content,
				 int version,
				 const std::string &languageId);
	void didEdit(const std::string &filePath, const std::string &content, int version);
	void didClose(const std::string &filePath);
	bool isDocumentOpen(const std::string &filePath) const;

	// Direct access to message handler
	lsp::MessageHandler *getMessageHandler() { return messageHandler.get(); }

	// Handle all LSP keybinds
	bool keybinds();

	// Point goto/hover/uri UI at a different editor (multi-tab embed).
	void bindEditorApi(EditorApi &api);

	// Render all LSP UI elements
	void render();

	// Server management
	bool startServer(const std::string &language, const std::string &serverPath);
	void stopServer();

	// Configuration management
	void initializeLanguageServers();

	// Path utilities
	std::string expandEnvironmentVariables(const std::string &path) const;

  private:
	// Helper functions
	std::string findServerPath(const std::string &language) const;
	std::string detectLanguageFromFile(const std::string &filePath) const;
	// Absolute/canonical path key so open/edit/close URIs match across hosts.
	static std::string documentKey(const std::string &filePath);
	bool sendLSPInitialize();
	void startMessageProcessingLoop();
	void messageProcessingThread();

	// State
	bool initialized;
	bool running;
	Settings *settings = nullptr;
	std::string workspacePath;
	std::string currentLanguage;
	// std::string serverArgs = "--log=error";
	std::string serverArgs = "";

	// Paths currently open on the server (documentKey form). Max open count is 1
	// per key; multi-tab embed keeps many keys, standalone replaces via close+open.
	std::unordered_set<std::string> openDocuments;

	// Language server configurations
	std::vector<LanguageServerInfo> languageServers;

	// LSP framework objects
	std::unique_ptr<lsp::Process> serverProcess;
	std::unique_ptr<lsp::Connection> connection;
	std::unique_ptr<lsp::MessageHandler> messageHandler;

	// Message processing thread
	std::thread processingThread;
};

#else // !NED_ENABLE_LSP

// Minimal stand-in so App/Ned/keybinds/settings compile without lsp-framework.
class LSPClient
{
  public:
	struct DashboardStub
	{
		void render() {}
		void setShow(bool) {}
	};

	LSPClient(EditorApi &api, FileExplorer &fileExplorer, Settings &settings);
	~LSPClient();

	DashboardStub dashboard;

	void setWorkspace(const std::string &workspacePath);
	bool init(const std::string &filePath);
	void shutdown();
	bool isInitialized() const { return false; }
	std::string getCurrentLanguage() const;

	const std::vector<LanguageServerInfo> &getLanguageServers() const;
	std::vector<std::string> getSupportedLanguages() const;

	void didOpen(const std::string &filePath,
				 const std::string &content,
				 int version,
				 const std::string &languageId);
	void didEdit(const std::string &filePath, const std::string &content, int version);
	void didClose(const std::string &filePath);
	bool isDocumentOpen(const std::string &filePath) const;

	bool keybinds();
	void bindEditorApi(EditorApi &api);
	void render();

	bool startServer(const std::string &language, const std::string &serverPath);
	void stopServer();
	void initializeLanguageServers();
	std::string expandEnvironmentVariables(const std::string &path) const;
};

#endif // NED_ENABLE_LSP
