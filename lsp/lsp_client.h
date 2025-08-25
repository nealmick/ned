#pragma once

#include <functional>
#include <future>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

// Forward declarations
namespace lsp {

class Connection;
class MessageHandler;
class Process;

namespace io {

class Stream;

}
} // namespace lsp

// Structure to hold language server information
struct LanguageServerInfo
{
	std::string language;
	std::vector<std::string> fileExtensions;
	std::vector<std::string> serverPaths;
	std::vector<std::string> serverArgs;
};

class LSPClient
{
  public:
	LSPClient();
	~LSPClient();

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

	// Document management
	void didOpen(const std::string &filePath, const std::string &content);
	void didEdit(const std::string &filePath, const std::string &content);

	// Direct access to message handler
	lsp::MessageHandler *getMessageHandler() { return messageHandler.get(); }

	// Handle all LSP keybinds
	bool keybinds();

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
	void startMessageProcessingLoop();
	void messageProcessingThread();

	// State
	bool initialized;
	bool running;
	std::string workspacePath;
	std::string currentLanguage;
	// std::string serverArgs = "--log=error";
	std::string serverArgs = "";

	// Language server configurations
	std::vector<LanguageServerInfo> languageServers;

	// LSP framework objects
	std::unique_ptr<lsp::Process> serverProcess;
	std::unique_ptr<lsp::Connection> connection;
	std::unique_ptr<lsp::MessageHandler> messageHandler;

	// Message processing thread
	std::thread processingThread;
};

// Global instance
extern LSPClient gLSPClient;
