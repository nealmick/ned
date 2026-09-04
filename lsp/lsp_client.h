#pragma once

#ifndef NED_ENABLE_LSP
#define NED_ENABLE_LSP 1
#endif

#include <string>
#include <vector>

class LspEditor;
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

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <thread>

#include "../editor/editor_events.h"
#include "../editor/services/diagnostics/diagnostics_store.h"
#include "lsp_document_sync.h"
#include "lsp_goto.h"
#include "lsp_hover.h"

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
	LSPClient(LspEditor &api, Settings &settings);
	~LSPClient();

	// Goto-definition/references + hover requests (results rendered by the
	// UI layer).
	LSPGoto gotoDef;
	LSPGoto gotoRef;
	LSPHover hover;

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

	// Point goto/hover requests at a different editor (multi-tab embed).
	// Null is valid: no focused editor (all tabs closed).
	void bindEditorApi(LspEditor *api);

	// Keybind lookup for the UI layer (ImGui/Qt views poll their own keys).
	const class KeybindsManager &settingsKeybinds() const;

	// Server management
	bool startServer(const std::string &language, const std::string &serverPath);
	void stopServer();

	// Configuration management
	void initializeLanguageServers();

	// Path utilities
	std::string expandEnvironmentVariables(const std::string &path) const;

  public:
	// Resolve the configured server path for a language ("" when none of
	// its candidates exists) — the dashboards probe through this so their
	// "found" state can never disagree with what startServer would launch.
	std::string findServerPath(const std::string &language) const;

  private:
	// Helper functions
	std::string detectLanguageFromFile(const std::string &filePath) const;
	bool sendLSPInitialize();
	void registerServerHandlers();
	void applyInitializeResult(const lsp::InitializeResult &result);
	void startMessageProcessingLoop();
	void messageProcessingThread();
	void startStderrDrain();
	void stopStderrDrain();
	void stderrDrainLoop();

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
	// All outbound messages flow through this queue so no caller (least of
	// all the UI thread) ever blocks on the server draining its stdin.
	std::unique_ptr<class QueuedStreamWriter> outbound;
	std::unique_ptr<lsp::Connection> connection;
	std::unique_ptr<lsp::MessageHandler> messageHandler;

	// Message processing thread
	std::thread processingThread;

	// Server stderr drain. The framework pipes the server's stderr into a
	// 64KB OS pipe that ONLY this loop reads — a server that logs more than
	// that (clangd's background-index logging does, over a long session)
	// blocks in write(2) forever and the whole JSON-RPC stream freezes while
	// both processes stay alive. Must be stopped before serverProcess is
	// touched (started/stopped around it in startServer/stopServer).
	std::thread stderrDrainThread;
	std::atomic<bool> drainingStderr{false};
};

#else // !NED_ENABLE_LSP

#include "../editor/editor_events.h"
#include "../editor/services/diagnostics/diagnostics_store.h"
#include <functional>

// Minimal stand-in so App/Ned/keybinds/settings compile without lsp-framework.
class LSPClient
{
  public:
	LSPClient(LspEditor &api, Settings &settings);
	~LSPClient();

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

	void bindEditorApi(LspEditor *api);
	const class KeybindsManager &settingsKeybinds() const;

	bool startServer(const std::string &language, const std::string &serverPath);
	void stopServer();
	void initializeLanguageServers();
	std::string expandEnvironmentVariables(const std::string &path) const;

  private:
	LSPDiagnostics diagnostics_;
};

#endif // NED_ENABLE_LSP
