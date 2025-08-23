#include "lsp_client.h"
#include "../.build/lib/lsp-framework/generated/lsp/messages.h"
#include "../.build/lib/lsp-framework/generated/lsp/types.h"
#include "../lib/lsp-framework/lsp/connection.h"
#include "../lib/lsp-framework/lsp/error.h"
#include "../lib/lsp-framework/lsp/messagehandler.h"
#include "../lib/lsp-framework/lsp/process.h"
#include "imgui.h"
#include "lsp_goto_def.h"
#include "lsp_symbol_info.h"

#include <algorithm>
#include <filesystem>
#include <iostream>

// Global instance
LSPClient gLSPClient;

LSPClient::LSPClient() : initialized(false), running(false) {}

LSPClient::~LSPClient() { shutdown(); }

void LSPClient::setWorkspace(const std::string &workspacePath)
{
	if (this->workspacePath != workspacePath)
	{
		// New workspace, reset everything
		shutdown();
		this->workspacePath = workspacePath;
		std::cout << "LSP: Set workspace to: " << workspacePath << std::endl;
	}
}

bool LSPClient::init(const std::string &filePath)
{
	if (initialized)
		return true; // Already initialized, don't reinitialize

	if (workspacePath.empty())
	{
		std::cout << "LSP: No workspace set, cannot initialize" << std::endl;
		return false;
	}

	// Detect language from this file
	std::string detectedLanguage = detectLanguageFromFile(filePath);
	if (detectedLanguage.empty())
	{
		// Not a recognized file type, don't initialize yet
		return false;
	}

	std::cout << "LSP: First recognized file: " << filePath
			  << " (language: " << detectedLanguage << ")" << std::endl;

	if (startServer(detectedLanguage, ""))
	{
		initialized = true;
		std::cout << "LSP: Successfully initialized for " << detectedLanguage
				  << std::endl;
		return true;
	}

	std::cout << "LSP: Failed to start server for " << detectedLanguage << std::endl;
	return false;
}

std::string LSPClient::detectLanguageFromFile(const std::string &filePath) const
{
	std::filesystem::path file(filePath);
	std::string extension = file.extension().string();

	// Convert to lowercase
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

	if (extension == ".cpp" || extension == ".cxx" || extension == ".cc" ||
		extension == ".c" || extension == ".h" || extension == ".hpp" ||
		extension == ".hxx")
	{
		return "cpp";
	}

	if (extension == ".ts" || extension == ".tsx" || extension == ".js" ||
		extension == ".jsx")
	{
		return "typescript";
	}

	if (extension == ".py" || extension == ".pyx")
	{
		return "python";
	}

	if (extension == ".go")
	{
		return "go";
	}

	return "";
}

std::string LSPClient::findServerPath(const std::string &language) const
{
	// Cross-platform server path detection
	std::vector<std::string> searchPaths;

	if (language == "cpp")
	{
		// Common clangd paths
		searchPaths = {"clangd",
					   "/usr/bin/clangd",
					   "/usr/local/bin/clangd",
					   "/opt/homebrew/bin/clangd",
					   "C:/Program Files/LLVM/bin/clangd.exe"};
	} else if (language == "typescript")
	{
		// TypeScript language server
		searchPaths = {
			"typescript-language-server",
			"/usr/bin/typescript-language-server",
			"/usr/local/bin/typescript-language-server",
			"/opt/homebrew/bin/typescript-language-server",
			"C:/Users/%USERNAME%/AppData/Roaming/npm/typescript-language-server.cmd"};
	} else if (language == "python")
	{
		// Pyright language server
		searchPaths = {"pyright-langserver",
					   "/usr/bin/pyright-langserver",
					   "/usr/local/bin/pyright-langserver",
					   "/opt/homebrew/bin/pyright-langserver",
					   "C:/Users/%USERNAME%/AppData/Roaming/npm/pyright-langserver.cmd"};
	} else if (language == "go")
	{
		// Go language server
		searchPaths = {"gopls",
					   "/usr/bin/gopls",
					   "/usr/local/bin/gopls",
					   "/opt/homebrew/bin/gopls",
					   "C:/Users/%USERNAME%/go/bin/gopls.exe"};
	}

	std::cout << "LSP: Checking paths for " << language << " server:" << std::endl;
	// Check if any of the paths exist and are executable
	for (const auto &path : searchPaths)
	{
		std::cout << "LSP:   Checking: " << path;
		if (std::filesystem::exists(path))
		{
			if (std::filesystem::is_regular_file(path))
			{
				std::cout << " - FOUND!" << std::endl;
				return path;
			} else
			{
				std::cout << " - exists but not a file" << std::endl;
			}
		} else
		{
			std::cout << " - not found" << std::endl;
		}
	}

	std::cout << "LSP: No server found for " << language << std::endl;
	return "";
}

bool LSPClient::startServer(const std::string &language, const std::string &serverPath)
{
	if (initialized)
	{
		std::cout << "LSP: Server already initialized for language: " << language
				  << std::endl;
		return true;
	}

	currentLanguage = language;
	std::string actualServerPath =
		serverPath.empty() ? findServerPath(language) : serverPath;

	std::cout << "LSP: Looking for server for language: " << language << std::endl;
	std::cout << "LSP: Server path: "
			  << (actualServerPath.empty() ? "not found" : actualServerPath) << std::endl;

	if (actualServerPath.empty())
	{
		std::cerr << "LSP: Could not find server for language: " << language << std::endl;
		return false;
	}

	// Create server process with appropriate arguments
	std::vector<std::string> args;
	if (language == "python")
	{
		args = {"--stdio"};
	} else if (language == "typescript")
	{
		args = {"--stdio"};
	}
	// gopls works in stdio mode by default

	// Add flags for all servers
	if (!serverArgs.empty())
	{
		args.push_back(serverArgs);
	}

	try
	{
		std::cout << "LSP: Creating server process..." << std::endl;
		serverProcess = std::make_unique<lsp::Process>(actualServerPath, args);

		std::cout << "LSP: Creating connection..." << std::endl;
		connection = std::make_unique<lsp::Connection>(serverProcess->stdIO());

		std::cout << "LSP: Creating message handler..." << std::endl;
		messageHandler = std::make_unique<lsp::MessageHandler>(*connection);

		// Send LSP initialize request
		if (!sendLSPInitialize())
		{
			std::cerr << "LSP: Failed to send initialize request" << std::endl;
			return false;
		}

		// Start message processing loop
		startMessageProcessingLoop();

		std::cout << "LSP: Server started successfully for " << language << std::endl;
		return true;

	} catch (const std::exception &e)
	{
		std::cerr << "LSP: Failed to start server: " << e.what() << std::endl;
		return false;
	}
}

void LSPClient::shutdown()
{
	if (initialized)
	{
		std::cout << "LSP: Beginning shutdown sequence..." << std::endl;

		// First send proper LSP shutdown to server
		stopServer();

		// Stop message processing loop
		running = false;

		// Give thread a chance to exit gracefully, but don't wait forever
		if (processingThread.joinable())
		{
			std::cout << "LSP: Waiting for message processing thread to exit..."
					  << std::endl;
			if (processingThread.joinable())
			{
				// Try to join with timeout - if it doesn't respond, we'll detach
				std::thread timeoutThread([this]() {
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					if (processingThread.joinable())
					{
						std::cout << "LSP: Thread didn't exit gracefully, detaching..."
								  << std::endl;
						processingThread.detach();
					}
				});

				processingThread.join();
				timeoutThread.detach(); // Cancel timeout if join succeeded
			}
		}

		initialized = false;
		std::cout << "LSP: Shutdown complete" << std::endl;
	}
}
void LSPClient::stopServer()
{
	if (messageHandler)
	{
		try
		{
			std::cout << "LSP: Sending shutdown request..." << std::endl;

			// Send LSP shutdown request (shutdown request has no parameters)
			auto shutdownResponse =
				messageHandler->sendRequest<lsp::requests::Shutdown>();

			// Wait briefly for shutdown response, but don't block indefinitely
			try
			{
				shutdownResponse.result.wait_for(std::chrono::milliseconds(500));
				std::cout << "LSP: Shutdown request completed" << std::endl;
			} catch (...)
			{
				std::cout << "LSP: Shutdown request timed out, proceeding anyway"
						  << std::endl;
			}

			// Send exit notification
			messageHandler->sendNotification<lsp::notifications::Exit>();
			std::cout << "LSP: Exit notification sent" << std::endl;

		} catch (const std::exception &e)
		{
			std::cout << "LSP: Error during shutdown: " << e.what() << std::endl;
		}
	}

	// Force cleanup regardless of LSP protocol completion
	messageHandler.reset();
	connection.reset();

	// Force terminate server process if it's still running
	if (serverProcess)
	{
		std::cout << "LSP: Terminating server process" << std::endl;
		serverProcess.reset();
	}

	currentLanguage.clear();
}

bool LSPClient::sendLSPInitialize()
{
	if (!messageHandler)
		return false;

	try
	{
		std::cout << "LSP: Sending initialize request..." << std::endl;

		// Create initialize params
		lsp::InitializeParams params;
		params.rootUri = lsp::FileUri::fromPath(workspacePath);

		// Set up client capabilities properly
		params.capabilities.textDocument = lsp::TextDocumentClientCapabilities{};
		params.capabilities.textDocument->hover = lsp::HoverClientCapabilities{};
		params.capabilities.textDocument->hover->dynamicRegistration = false;

		// Send initialize request
		auto response =
			messageHandler->sendRequest<lsp::requests::Initialize>(std::move(params));

		// Handle response asynchronously to avoid blocking
		std::thread([this, future = std::move(response.result)]() mutable {
			try
			{
				auto result = future.get();
				std::cout << "LSP: Initialize request completed successfully"
						  << std::endl;

				// Send initialized notification
				lsp::InitializedParams initParams;
				messageHandler->sendNotification<lsp::notifications::Initialized>(
					std::move(initParams));

				std::cout << "LSP: Server is now initialized and ready" << std::endl;
			} catch (const std::exception &e)
			{
				std::cerr << "LSP: Initialize response failed: " << e.what() << std::endl;
			}
		}).detach();

		return true;

	} catch (const std::exception &e)
	{
		std::cerr << "LSP: Initialize request failed: " << e.what() << std::endl;
		return false;
	}
}

void LSPClient::didOpen(const std::string &filePath, const std::string &content)
{
	if (!initialized || !messageHandler)
		return;

	try
	{
		// Create didOpen notification
		lsp::DidOpenTextDocumentParams params;
		params.textDocument.uri = lsp::FileUri::fromPath(filePath);
		params.textDocument.languageId = "cpp"; // Simplified for now
		params.textDocument.version = 1;
		params.textDocument.text = content;

		// Send the notification
		messageHandler->sendNotification<lsp::notifications::TextDocument_DidOpen>(
			std::move(params));

		std::cout << "LSP: Sending didOpen for file: " << filePath << std::endl;
		std::cout << "LSP: Document opened: " << filePath
				  << " (content length: " << content.length() << ")" << std::endl;
	} catch (const std::exception &e)
	{
		std::cerr << "LSP: Failed to send didOpen: " << e.what() << std::endl;
	}
}

void LSPClient::didEdit(const std::string &filePath, const std::string &content)
{
	if (!initialized || !messageHandler)
		return;

	try
	{
		// Create didChange notification
		lsp::DidChangeTextDocumentParams params;
		params.textDocument.uri = lsp::FileUri::fromPath(filePath);
		params.textDocument.version = 2; // Increment version for changes

		// Create a content change that replaces the entire document
		lsp::TextDocumentContentChangeEvent_Text change;
		change.text = content;
		params.contentChanges.push_back(change);

		// Send the notification
		messageHandler->sendNotification<lsp::notifications::TextDocument_DidChange>(
			std::move(params));

		// std::cout << "LSP: Sending didChange for file: " << filePath << std::endl;
	} catch (const std::exception &e)
	{
		std::cerr << "LSP: Failed to send didChange: " << e.what() << std::endl;
	}
}

void LSPClient::startMessageProcessingLoop()
{
	if (!messageHandler || running)
		return;

	running = true;
	processingThread = std::thread(&LSPClient::messageProcessingThread, this);
	std::cout << "LSP: Message processing thread started" << std::endl;
}

void LSPClient::messageProcessingThread()
{
	std::cout << "LSP: Message processing thread running" << std::endl;

	try
	{
		while (running && messageHandler)
		{
			messageHandler->processIncomingMessages();
		}
	} catch (const std::exception &e)
	{
		std::cerr << "LSP: Message processing thread error: " << e.what() << std::endl;
	}

	std::cout << "LSP: Message processing thread exiting" << std::endl;
}

bool LSPClient::keybinds()
{
	if (!initialized)
		return false;

	bool modPressed = ImGui::GetIO().KeyCtrl;
	if (!modPressed)
		return false;

	bool shortcutPressed = false;

	// LSP Symbol Info keybind (Ctrl+i)
	if (ImGui::IsKeyPressed(ImGuiKey_I, false))
	{
		gLSPSymbolInfo.get();
		shortcutPressed = true;
	}

	// LSP Goto Definition keybind (Ctrl+d)
	if (ImGui::IsKeyPressed(ImGuiKey_D, false))
	{
		gLSPGotoDef.get();
		shortcutPressed = true;
	}

	return shortcutPressed;
}

void LSPClient::render()
{
	gLSPSymbolInfo.render();
	gLSPGotoDef.render();
}