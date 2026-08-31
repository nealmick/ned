#include "lsp_client.h"
#include "../editor/editor_api.h"
#include "../files/files.h"
#include "../util/keybinds.h"
#include "../util/settings.h"
#include "lsp_includes.h"
#include "lsp_trace.h"

#include "lsp_goto.h"

#include "lsp_symbol_info.h"

#include "../lib/json.hpp"
#include "imgui.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace {

// Single workspace folder for the current workspace (empty array if unset).
lsp::Array<lsp::WorkspaceFolder> workspaceFoldersFor(const std::string &workspacePath)
{
	lsp::Array<lsp::WorkspaceFolder> folders;
	if (workspacePath.empty())
		return folders;
	lsp::WorkspaceFolder folder;
	folder.uri = lsp::Uri::fileUriFromPath(workspacePath);
	folder.name = std::filesystem::path(workspacePath).filename().string();
	if (folder.name.empty())
		folder.name = workspacePath;
	folders.push_back(std::move(folder));
	return folders;
}

} // namespace

LSPClient::LSPClient(EditorApi &api, FileExplorer &fileExplorer, Settings &settings)
	: dashboard(*this, fileExplorer, settings),
	  gotoDef(*this, api, LSPGoto::Kind::Definition),
	  gotoRef(*this, api, LSPGoto::Kind::References),
	  symbolInfo(*this, api),
	  uriOptions(api, fileExplorer, settings),
	  initialized(false),
	  running(false),
	  settings(&settings),
	  sync(diagnostics_,
		   [this](const std::string &path) { return detectLanguageFromFile(path); })
{
	initializeLanguageServers();
	dashboard.refreshServerInfo();
}

LSPClient::~LSPClient() { shutdown(); }

void LSPClient::bindEditorApi(EditorApi &api)
{
	gotoDef.setApi(api);
	gotoRef.setApi(api);
	symbolInfo.setApi(api);
	uriOptions.setApi(api);
	setHoverApi(api);
}

void LSPClient::setHoverApi(EditorApi &api) { symbolInfo.setHoverApi(api); }

void LSPClient::setWorkspace(const std::string &workspacePath)
{
	if (this->workspacePath != workspacePath)
	{
		// New workspace, reset everything
		shutdown();
		this->workspacePath = workspacePath;
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

	//	<< " (language: " << detectedLanguage << ")" << std::endl;

	if (startServer(detectedLanguage, ""))
		return true;

	std::cout << "LSP: Failed to start server for " << detectedLanguage << std::endl;
	return false;
}

std::string LSPClient::detectLanguageFromFile(const std::string &filePath) const
{
	std::filesystem::path file(filePath);
	std::string extension = file.extension().string();

	// Convert to lowercase
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

	// Search through configured language servers
	for (const auto &server : languageServers)
	{
		for (const auto &ext : server.fileExtensions)
		{
			if (extension == ext)
			{
				return server.language;
			}
		}
	}

	return "";
}

std::string LSPClient::expandEnvironmentVariables(const std::string &path) const
{
#ifdef _WIN32
	// Expand Windows environment variables like %USERNAME%
	std::string result = path;
	size_t pos = 0;
	while ((pos = result.find('%', pos)) != std::string::npos)
	{
		size_t end = result.find('%', pos + 1);
		if (end == std::string::npos)
			break;

		std::string varName = result.substr(pos + 1, end - pos - 1);
		char *envVar = getenv(varName.c_str());
		if (envVar != nullptr)
		{
			result.replace(pos, end - pos + 1, envVar);
			pos += strlen(envVar);
		} else
		{
			pos = end + 1;
		}
	}
	return result;
#else
	return path;
#endif
}

std::string LSPClient::findServerPath(const std::string &language) const
{
	// Find the language server configuration
	const LanguageServerInfo *serverInfo = nullptr;
	for (const auto &server : languageServers)
	{
		if (server.language == language)
		{
			serverInfo = &server;
			break;
		}
	}

	if (!serverInfo)
	{
		std::cout << "LSP: No server configuration found for " << language << std::endl;
		return "";
	}

	// Check if any of the paths exist and are executable
	for (const auto &path : serverInfo->serverPaths)
	{
		std::string expandedPath = expandEnvironmentVariables(path);
		if (std::filesystem::exists(expandedPath))
		{
			if (std::filesystem::is_regular_file(expandedPath))
			{
				std::cout << " - FOUND!" << std::endl;
				return expandedPath;
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
		//			  << std::endl;
		return true;
	}

	currentLanguage = language;
	std::string actualServerPath =
		serverPath.empty() ? findServerPath(language) : serverPath;

	//	<< (actualServerPath.empty() ? "not found" : actualServerPath) << std::endl;

	if (actualServerPath.empty())
	{
		std::cerr << "LSP: Could not find server for language: " << language << std::endl;
		return false;
	}

	// Create server process with appropriate arguments
	std::vector<std::string> args;

	// Find server configuration and use its arguments
	for (const auto &server : languageServers)
	{
		if (server.language == language)
		{
			args = server.serverArgs; // Copy the server-specific args
			break;
		}
	}

	// Add global flags for all servers
	if (!serverArgs.empty())
	{
		args.push_back(serverArgs);
	}

	try
	{
		serverProcess = std::make_unique<lsp::Process>(actualServerPath, args);

		connection = std::make_unique<lsp::Connection>(serverProcess->stdIO());

		messageHandler = std::make_unique<lsp::MessageHandler>(*connection);
		sync.connect(*messageHandler);
		registerServerHandlers();

		if (!sendLSPInitialize())
		{
			std::cerr << "LSP: Failed to send initialize request" << std::endl;
			return false;
		}

		startMessageProcessingLoop();
		initialized = true;
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

		// Give thread a chance to exit gracefully, but don't wait forever on Windows
		if (processingThread.joinable())
		{
			std::cout << "LSP: Waiting for message processing thread to exit..."
					  << std::endl;

#ifdef _WIN32
			// On Windows, just detach immediately to avoid hanging
			std::cout << "LSP: Detaching thread on Windows to prevent hang..."
					  << std::endl;
			processingThread.detach();
#else
			// On Unix systems, try to join with a reasonable timeout
			auto future = std::async(std::launch::async, [this]() {
				if (processingThread.joinable())
				{
					processingThread.join();
				}
			});

			if (future.wait_for(std::chrono::milliseconds(1000)) ==
				std::future_status::timeout)
			{
				std::cout << "LSP: Thread didn't exit gracefully, detaching..."
						  << std::endl;
				if (processingThread.joinable())
				{
					processingThread.detach();
				}
			}
#endif
		}

		initialized = false;
		sync.disconnect();
		diagnostics_.clearAll();
		std::cout << "LSP: Shutdown complete" << std::endl;
	} else
	{
		sync.disconnect();
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
	sync.disconnect();

	// Force terminate server process if it's still running
	if (serverProcess)
	{
		std::cout << "LSP: Terminating server process" << std::endl;
		serverProcess.reset();
	}

	currentLanguage.clear();
}

void LSPClient::registerServerHandlers()
{
	if (!messageHandler)
		return;

	messageHandler->add<lsp::notifications::TextDocument_PublishDiagnostics>(
		[this](lsp::PublishDiagnosticsParams &&params) {
			std::vector<DiagnosticItem> items;
			items.reserve(params.diagnostics.size());
			for (const auto &d : params.diagnostics)
			{
				DiagnosticItem item;
				item.startLine = static_cast<int>(d.range.start.line);
				item.startCharacter = static_cast<int>(d.range.start.character);
				item.endLine = static_cast<int>(d.range.end.line);
				item.endCharacter = static_cast<int>(d.range.end.character);
				// 3.18: message is String | MarkupContent. Flatten markup to
				// plain text — the tooltip renderer re-parses markdown anyway.
				if (std::holds_alternative<lsp::String>(d.message))
					item.message = std::get<lsp::String>(d.message);
				else if (std::holds_alternative<lsp::MarkupContent>(d.message))
					item.message = std::get<lsp::MarkupContent>(d.message).value;
				if (d.severity)
					item.severity = static_cast<int>(d.severity->value());
				if (d.source)
					item.source = *d.source;
				items.push_back(std::move(item));
			}
			int version = -1;
			if (params.version)
				version = *params.version;
			diagnostics_.replace(
				std::string(params.uri.fsPath()), std::move(items), version);
		});

	messageHandler->add<lsp::requests::Workspace_Configuration>(
		[](lsp::ConfigurationParams &&params) {
			lsp::Workspace_ConfigurationResult out;
			out.reserve(params.items.size());
			for (size_t i = 0; i < params.items.size(); ++i)
				out.emplace_back(lsp::json::Object{});
			return out;
		});

	messageHandler->add<lsp::requests::Workspace_WorkspaceFolders>([this]() {
		return lsp::Workspace_WorkspaceFoldersResult{workspaceFoldersFor(workspacePath)};
	});

	messageHandler->add<lsp::requests::Client_RegisterCapability>(
		[](lsp::RegistrationParams &&) { return nullptr; });

	messageHandler->add<lsp::requests::Window_WorkDoneProgress_Create>(
		[](lsp::WorkDoneProgressCreateParams &&) { return nullptr; });

	messageHandler->add<lsp::requests::Window_ShowMessageRequest>(
		[](lsp::ShowMessageRequestParams &&) {
			return lsp::Window_ShowMessageRequestResult{nullptr};
		});
}

void LSPClient::applyInitializeResult(const lsp::InitializeResult &result)
{
	sync.applyCapabilities(result);
}

bool LSPClient::sendLSPInitialize()
{
	if (!messageHandler)
		return false;

	try
	{
		lsp::InitializeParams params;
#ifdef _WIN32
		params.processId = _getpid();
#else
		params.processId = static_cast<int>(getpid());
#endif
		params.rootUri = lsp::Uri::fileUriFromPath(workspacePath);
		params.rootPath = workspacePath;
		params.clientInfo = lsp::ClientInfo{};
		params.clientInfo->name = "ned";
		params.workspaceFolders = workspaceFoldersFor(workspacePath);

		params.capabilities.workspace = lsp::WorkspaceClientCapabilities{};
		params.capabilities.workspace->workspaceFolders = true;
		params.capabilities.workspace->configuration = true;

		params.capabilities.textDocument = lsp::TextDocumentClientCapabilities{};
		auto &td = *params.capabilities.textDocument;
		td.synchronization = lsp::TextDocumentSyncClientCapabilities{};
		td.synchronization->didSave = true;
		td.hover = lsp::HoverClientCapabilities{};
		td.definition = lsp::DefinitionClientCapabilities{};
		td.definition->linkSupport = true;
		td.references = lsp::ReferenceClientCapabilities{};
		td.publishDiagnostics = lsp::PublishDiagnosticsClientCapabilities{};
		td.publishDiagnostics->relatedInformation = true;

		params.capabilities.general = lsp::GeneralClientCapabilities{};
		params.capabilities.general->positionEncodings = {
			lsp::PositionEncodingKind::UTF16};

		messageHandler->sendRequest<lsp::requests::Initialize>(
			std::move(params),
			[this](lsp::InitializeResult &&result) {
				applyInitializeResult(result);
				lsp::InitializedParams initParams;
				messageHandler->sendNotification<lsp::notifications::Initialized>(
					std::move(initParams));
				NED_LSP_TRACE("handshake ready");
				sync.markHandshakeReady();
			},
			[](const lsp::ResponseError &error) {
				std::cerr << "LSP: Initialize failed: " << error.message() << std::endl;
			});

		return true;
	} catch (const std::exception &e)
	{
		std::cerr << "LSP: Initialize request failed: " << e.what() << std::endl;
		return false;
	}
}

bool LSPClient::isDocumentOpen(const std::string &filePath) const
{
	return sync.isDocumentOpen(filePath);
}

void LSPClient::didOpen(const std::string &filePath,
						const std::string &content,
						int version,
						const std::string &languageId)
{
	// Gating (handshake state, handler) lives in LSPDocumentSync.
	sync.didOpen(filePath, content, version, languageId);
}

void LSPClient::didChange(const std::string &filePath,
						  int version,
						  const std::vector<EditorEvents::DocumentChange> &changes,
						  const FullTextProvider &fullText)
{
	sync.didChange(filePath, version, changes, fullText);
}

void LSPClient::didSave(const std::string &filePath, const FullTextProvider &fullText)
{
	sync.didSave(filePath, fullText);
}

void LSPClient::didClose(const std::string &filePath) { sync.didClose(filePath); }

void LSPClient::startMessageProcessingLoop()
{
	if (!messageHandler || running)
		return;

	running = true;
	// A previous reader thread that died with the server has finished but not
	// been joined — assigning over a joinable thread would terminate().
	if (processingThread.joinable())
		processingThread.join();
	processingThread = std::thread(&LSPClient::messageProcessingThread, this);
}

void LSPClient::messageProcessingThread()
{
	// processIncomingMessages reads ONE message (blocking) and returns — the
	// loop is the read loop. Malformed messages throw and are skipped (one
	// bad server message must not kill hover/diagnostics); a ConnectionError
	// means the pipe is gone — bail immediately.
	bool connectionLost = false;
	constexpr int kMaxConsecutiveFailures = 16;
	int failures = 0;

	while (running && messageHandler)
	{
		try
		{
			messageHandler->processIncomingMessages();
			NED_LSP_TRACE("processed incoming message");
			failures = 0;
		} catch (const lsp::ConnectionError &)
		{
			std::cerr << "LSP: Server connection lost" << std::endl;
			connectionLost = true;
			break;
		} catch (const std::exception &e)
		{
			std::cerr << "LSP: Ignoring malformed server message: " << e.what()
					  << std::endl;
			if (++failures >= kMaxConsecutiveFailures)
			{
				std::cerr << "LSP: Connection unusable, stopping message loop"
						  << std::endl;
				connectionLost = true;
				break;
			}
		}
	}

	// Died mid-session (not a deliberate shutdown()): reset the handshake
	// state so isInitialized() goes false — the dashboard then shows the
	// server as Inactive instead of a stale "Active", feature gates stop
	// sending requests into the dead pipe, and a later init() (next file
	// open) can start a fresh server.
	if (connectionLost && running)
	{
		sync.disconnect();
		initialized = false;
		running = false; // allow startMessageProcessingLoop on restart
	}
}

bool LSPClient::keybinds()
{
	if (!initialized)
		return false;

	bool modPressed = ImGui::GetIO().KeyCtrl;
	if (!modPressed)
		return false;

	bool shortcutPressed = false;

	// LSP Symbol Info keybind
	if (!settings)
		return false;

	ImGuiKey symbolInfoKey = settings->keybinds.getActionKey("lsp_symbol_info");
	if (symbolInfoKey != ImGuiKey_None && ImGui::IsKeyPressed(symbolInfoKey, false))
	{
		symbolInfo.get();
		shortcutPressed = true;
	}

	// LSP Goto Definition keybind
	ImGuiKey gotoDefKey = settings->keybinds.getActionKey("lsp_find_def");
	if (gotoDefKey != ImGuiKey_None && ImGui::IsKeyPressed(gotoDefKey, false))
	{
		gotoDef.get();
		shortcutPressed = true;
	}

	// LSP Goto References keybind
	ImGuiKey gotoRefKey = settings->keybinds.getActionKey("lsp_find_ref");
	if (gotoRefKey != ImGuiKey_None && ImGui::IsKeyPressed(gotoRefKey, false))
	{
		gotoRef.get();
		shortcutPressed = true;
	}

	return shortcutPressed;
}

void LSPClient::render()
{
	symbolInfo.render();
	gotoDef.render();
	gotoRef.render();
}

void LSPClient::initializeLanguageServers()
{
	languageServers.clear();

	std::string lspJsonPath =
		(std::filesystem::path(Settings::getUserConfigDir()) / "lsp.json").string();
	std::ifstream file(lspJsonPath);
	if (!file.is_open())
	{
		std::cerr << "[LSP] Failed to open lsp.json file: " << lspJsonPath << std::endl;
		return;
	}

	nlohmann::json jsonData;
	try
	{
		file >> jsonData;
	} catch (const std::exception &e)
	{
		std::cerr << "[LSP] Failed to parse lsp.json: " << e.what() << std::endl;
		return;
	}

	if (!jsonData.contains("languages") || !jsonData["languages"].is_array())
	{
		std::cerr << "[LSP] Invalid lsp.json format: missing 'languages' array"
				  << std::endl;
		return;
	}

	for (const auto &lang : jsonData["languages"])
	{
		if (!lang.contains("language_name") ||
			!lang.contains("language_file_extensions") ||
			!lang.contains("language_server_paths"))
		{
			std::cerr << "[LSP] Skipping invalid language entry in lsp.json" << std::endl;
			continue;
		}

		LanguageServerInfo serverInfo;
		serverInfo.language = lang["language_name"];
		serverInfo.fileExtensions = lang["language_file_extensions"];
		serverInfo.serverPaths = lang["language_server_paths"];

		// Default args based on language
		if (serverInfo.language == "typescript" || serverInfo.language == "python")
		{
			serverInfo.serverArgs = {"--stdio"};
		} else
		{
			serverInfo.serverArgs = {};
		}

		languageServers.push_back(serverInfo);
	}

	std::cout << "[LSP] Loaded " << languageServers.size()
			  << " language servers from lsp.json" << std::endl;
}

std::vector<std::string> LSPClient::getSupportedLanguages() const
{
	std::vector<std::string> languages;
	for (const auto &server : languageServers)
	{
		languages.push_back(server.language);
	}
	return languages;
}
