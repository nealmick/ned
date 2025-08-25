#include "lsp_dashboard.h"
#include "../files/files.h"
#include "../util/settings.h"
#include "../util/settings_file_manager.h"
#include "imgui.h"
#include "lsp_client.h"
#include <filesystem>
#include <iostream>

// Global instance
LSPDashboard gLSPDashboard;

LSPDashboard::LSPDashboard() { refreshServerInfo(); }

LSPDashboard::~LSPDashboard() {}

void LSPDashboard::render()
{
	if (!show)
		return;

	// Set window position and size on first use
	ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);

	bool windowOpen = true;
	bool windowCreated =
		ImGui::Begin("LSP Server Dashboard", &windowOpen, ImGuiWindowFlags_NoCollapse);

	if (windowCreated)
	{
		// Store current window position and size
		windowPos = ImGui::GetWindowPos();
		windowSize = ImGui::GetWindowSize();

		// If window was closed via X button, hide the dashboard
		if (!windowOpen)
		{
			show = false;
		}

		// Header
		ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f),
						   "Language Server Protocol Dashboard");
		ImGui::Separator();

		// Refresh button
		if (ImGui::Button("Refresh Server Status"))
		{
			refreshServerInfo();
		}

		ImGui::SameLine();
		if (ImGui::Button("Reload LSP.json"))
		{
			gLSPClient.initializeLanguageServers();
			refreshServerInfo();

			// Show notification with server count
			extern Settings gSettings;
			std::string message = "LSP Servers: " + std::to_string(serverInfos.size());
			gSettings.renderNotification(message, 2.0f);
		}

		ImGui::SameLine();
		if (ImGui::Button("Open LSP.json"))
		{
			std::string lspJsonPath =
				(std::filesystem::path(SettingsFileManager::getUserSettingsPath())
					 .parent_path() /
				 "lsp.json")
					.string();
			if (std::filesystem::exists(lspJsonPath))
			{
				gFileExplorer.loadFileContent(lspJsonPath);
				show = false; // Close LSP dashboard after opening file
			} else
			{
				std::cerr << "[LSP Dashboard] LSP.json file not found at: " << lspJsonPath
						  << std::endl;
			}
		}

		ImGui::Text("%zu servers configured", serverInfos.size());
		ImGui::Spacing();

		// Render server list
		renderServerList();

		// Handle window input (click outside to close)
		handleWindowInput();
	}

	ImGui::End();
}

void LSPDashboard::renderServerList()
{
	// Create a scrolling region for the server list
	ImGui::BeginChild("ServerList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);

	if (serverInfos.empty())
	{
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No LSP servers configured");
	} else
	{
		// Use modern ImGui table API instead of old columns
		if (ImGui::BeginTable("ServerTable",
							  4,
							  ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
								  ImGuiTableFlags_Resizable))
		{
			// Setup columns - all resizable
			ImGui::TableSetupColumn("Language", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			ImGui::TableSetupColumn("Server Path", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Found", ImGuiTableColumnFlags_WidthFixed, 140.0f);
			ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 180.0f);
			ImGui::TableHeadersRow();

			// Render each server entry
			for (const auto &serverInfo : serverInfos)
			{
				renderServerEntry(serverInfo);
			}

			ImGui::EndTable();
		}
	}

	ImGui::EndChild();
}

void LSPDashboard::renderServerEntry(const LSPServerInfo &serverInfo)
{
	ImGui::TableNextRow();

	// Language column
	ImGui::TableSetColumnIndex(0);
	ImGui::Text("%s", serverInfo.language.c_str());

	// Server path column (truncate if too long)
	ImGui::TableSetColumnIndex(1);
	std::string displayPath = serverInfo.serverPath;
	if (displayPath.length() > 40)
	{
		displayPath = "..." + displayPath.substr(displayPath.length() - 37);
	}
	ImGui::Text("%s", displayPath.c_str());
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%s", serverInfo.serverPath.c_str());
	}

	// Found status with green dot
	ImGui::TableSetColumnIndex(2);
	if (serverInfo.isFound)
	{
		ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "● Found");
	} else
	{
		ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "● Missing");
	}

	// Active status
	ImGui::TableSetColumnIndex(3);
	if (serverInfo.isFound)
	{
		if (serverInfo.isActive)
		{
			ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "● Active");
		} else
		{
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "● Inactive");
		}
	} else
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "N/A");
	}
}

void LSPDashboard::refreshServerInfo()
{
	serverInfos.clear();

	// Get language server configurations directly from the LSP client
	const auto &languageServers = gLSPClient.getLanguageServers();

	for (const auto &serverConfig : languageServers)
	{
		LSPServerInfo info;
		info.language = serverConfig.language;

		// Try to find the server using the same paths the LSP client uses
		std::string serverPath = "";
		for (const auto &path : serverConfig.serverPaths)
		{
			// Expand environment variables like %USERNAME% on Windows
			std::string expandedPath = gLSPClient.expandEnvironmentVariables(path);
			if (std::filesystem::exists(expandedPath) &&
				std::filesystem::is_regular_file(expandedPath))
			{
				serverPath = expandedPath;
				break;
			}
		}

		info.serverPath = serverPath.empty() ? "Not found" : serverPath;
		info.isFound = !serverPath.empty();

		// A server is active only if:
		// 1. The server exists/is found
		// 2. The global LSP client is initialized
		// 3. The current language matches this server's language
		info.isActive = info.isFound && gLSPClient.isInitialized() &&
						gLSPClient.getCurrentLanguage() == serverConfig.language;

		serverInfos.push_back(info);
	}
}

std::vector<std::string> LSPDashboard::getSupportedLanguages()
{
	// Use the LSP client's supported languages directly
	return gLSPClient.getSupportedLanguages();
}

void LSPDashboard::handleWindowInput()
{
	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		show = false;
	}

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		ImVec2 mousePos = ImGui::GetMousePos();
		ImVec2 windowPos = ImGui::GetWindowPos();
		ImVec2 windowSize = ImGui::GetWindowSize();
		if (mousePos.x < windowPos.x || mousePos.x > windowPos.x + windowSize.x ||
			mousePos.y < windowPos.y || mousePos.y > windowPos.y + windowSize.y)
		{
			if (!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId))
			{
				show = false;
			}
		}
	}
}