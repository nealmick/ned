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
#include <memory>
#include <thread>

#include "../editor/editor_events.h"
#include "../editor/services/diagnostics/diagnostics_store.h"
#include "lsp_dashboard.h"
#include "lsp_document_sync.h"
#include "lsp_goto.h"
#include "lsp_symbol_info.h"
#include "lsp_uri_options.h"

// Forward declarations
namespace lsp {

class Connection;
class MessageHandler;
class Process;
struct InitializeResult;

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
	LSPGoto gotoDef;
	LSPGoto gotoRef;
	LSPSymbolInfo symbolInfo;
	// Shared URI-options picker
	LSPUriOptions uriOptions;

	// Core LSP functionality
	void setWorkspace(const std::string &workspacePath);
	bool init(const std::string &filePath);
	void shutdown();
	bool isInitialized() const { return sync.isReady(); }
	std::string getCurrentLanguage() const { return currentLanguage; }
	LSPDiagnostics &diagnostics() { return diagnostics_; }
	const LSPDiagnostics &diagnostics() const { return diagnostics_; }

	// Language server information access
	const std::vector<LanguageServerInfo> &getLanguageServers() const
	{
		return languageServers;
	}
	std::vector<std::string> getSupportedLanguages() const;

	// Document management. Delegates to LSPDocumentSync — see that header for
	// open/close balance and lazy full-text semantics.
	using FullTextProvider = LSPDocumentSync::FullTextProvider;
	void didOpen(const std::string &filePath,
				 const std::string &content,
				 int version,
				 const std::string &languageId);
	void didChange(const std::string &filePath,
				   int version,
				   const std::vector<EditorEvents::DocumentChange> &changes,
				   const FullTextProvider &fullText);
	void didSave(const std::string &filePath, const FullTextProvider &fullText);
	void didClose(const std::string &filePath);
	bool isDocumentOpen(const std::string &filePath) const;

	// Direct access to message handler
	lsp::MessageHandler *getMessageHandler() { return messageHandler.get(); }

	// Handle all LSP keybinds
	bool keybinds();

	// Point goto/hover/uri UI at a different editor (multi-tab embed).
	void bindEditorApi(EditorApi &api);
	// Mouse-hover tooltip targets the editor under the mouse (splits differ
	// from the focused editor).
	void setHoverApi(EditorApi &api);

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
	bool sendLSPInitialize();
	void registerServerHandlers();
	void applyInitializeResult(const lsp::InitializeResult &result);
	void startMessageProcessingLoop();
	void messageProcessingThread();

	// State
	bool initialized = false; // process started
	bool running = false;
	Settings *settings = nullptr;
	std::string workspacePath;
	std::string currentLanguage;
	std::string serverArgs = "";

	LSPDiagnostics diagnostics_;
	LSPDocumentSync sync;

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

#include "../editor/editor_events.h"
#include "../editor/services/diagnostics/diagnostics_store.h"
#include <functional>

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

	using FullTextProvider = std::function<std::string()>;
	void didOpen(const std::string &filePath,
				 const std::string &content,
				 int version,
				 const std::string &languageId);
	void didChange(const std::string &filePath,
				   int version,
				   const std::vector<EditorEvents::DocumentChange> &changes,
				   const FullTextProvider &fullText);
	void didSave(const std::string &filePath, const FullTextProvider &fullText);
	void didClose(const std::string &filePath);
	bool isDocumentOpen(const std::string &filePath) const;

	LSPDiagnostics &diagnostics();
	const LSPDiagnostics &diagnostics() const;

	bool keybinds();
	void bindEditorApi(EditorApi &api);
	void setHoverApi(EditorApi &api);
	void render();

	bool startServer(const std::string &language, const std::string &serverPath);
	void stopServer();
	void initializeLanguageServers();
	std::string expandEnvironmentVariables(const std::string &path) const;

  private:
	LSPDiagnostics diagnostics_;
};

#endif // NED_ENABLE_LSP
