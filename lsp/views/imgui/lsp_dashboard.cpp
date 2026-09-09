#include "lsp_dashboard.h"
#include "../../../files/files.h"
#include "../../../util/settings.h"
#include "../../lsp_client.h"
#include "../../lsp_server_status.h"
#include "imgui.h"
#include <filesystem>
#include <iostream>

LSPDashboard::LSPDashboard(LSPClient &client,
						   FileExplorer &fileExplorer,
						   Settings &settings)
	: client(&client), fileExplorer(&fileExplorer), settings(&settings)
{
}

LSPDashboard::~LSPDashboard() {}

void LSPDashboard::render()
{
	if (!show || !client || !fileExplorer || !settings)
		return;

	const float fs = ImGui::GetFontSize();
	if (windowSize.x < fs * 20.0f)
		windowSize = ImVec2(fs * 40.0f, fs * 25.0f);
	ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);

	// Standalone uses a translucent global WindowBg for the host window glass effect.
	// Floating panels need an opaque bg so content is readable (same as Settings / URI
	// options).
	const bool pushBg = !settings->isEmbedded &&
						settings->settings.contains("backgroundColor") &&
						settings->settings["backgroundColor"].is_array() &&
						settings->settings["backgroundColor"].size() >= 3;
	if (pushBg)
	{
		const auto &bg = settings->settings["backgroundColor"];
		const float m = 0.8f;
		const ImVec4 windowBg(
			bg[0].get<float>() * m, bg[1].get<float>() * m, bg[2].get<float>() * m, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, windowBg);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, windowBg);
	}

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
			refresh();
		}

		ImGui::SameLine();
		if (ImGui::Button("Reload LSP.json"))
		{
			client->initializeLanguageServers();
			refresh();

			// Show notification with server count
			std::string message = "LSP Servers: " + std::to_string(serverInfos.size());
			settings->showNotification(message, 2.0f);
		}

		ImGui::SameLine();
		if (ImGui::Button("Open LSP.json"))
		{
			std::string lspJsonPath =
				(std::filesystem::path(Settings::getUserConfigDir()) / "lsp.json").string();
			if (std::filesystem::exists(lspJsonPath))
			{
				fileExplorer->loadFileContent(lspJsonPath);
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
	if (pushBg)
		ImGui::PopStyleColor(2);
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
	switch (lspFoundStatus(serverInfo).color)
	{
	case LSPStatusColorRole::Positive:
		ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "● Found");
		break;
	default:
		ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "● Missing");
		break;
	}

	// Active status
	ImGui::TableSetColumnIndex(3);
	const LSPServerStatus status = lspActiveStatus(serverInfo);
	ImVec4 statusColor(0.5f, 0.5f, 0.5f, 1.0f);
	if (status.color == LSPStatusColorRole::Positive)
		statusColor = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
	else if (status.color == LSPStatusColorRole::Muted)
		statusColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
	ImGui::TextColored(statusColor, "%s", status.text);
}

void LSPDashboard::refresh()
{
	if (!client)
	{
		serverInfos.clear();
		return;
	}
	serverInfos = probeLspServers(*client);
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
