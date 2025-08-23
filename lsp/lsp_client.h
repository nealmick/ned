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

	// Document management
	void didOpen(const std::string &filePath, const std::string &content);

	// Direct access to message handler
	lsp::MessageHandler *getMessageHandler() { return messageHandler.get(); }

	// Server management
	bool startServer(const std::string &language, const std::string &serverPath);
	void stopServer();

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

	// LSP framework objects
	std::unique_ptr<lsp::Process> serverProcess;
	std::unique_ptr<lsp::Connection> connection;
	std::unique_ptr<lsp::MessageHandler> messageHandler;

	// Message processing thread
	std::thread processingThread;
};

// Global instance
extern LSPClient gLSPClient;
