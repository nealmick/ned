/*
File: editor_header.cpp
Description: Editor header rendering implementation for NED text editor.
*/

#include "editor_header.h"
#include "ai/ai_tab.h"
#include "editor/editor_git.h"
#include "files/files.h"
#include "imgui.h"
#include "util/settings.h"
#include "util/terminal.h"

// External dependencies
extern class FileExplorer gFileExplorer;
extern class Settings gSettings;
extern class AITab gAITab;
extern class EditorGit gEditorGit;

namespace fs = std::filesystem;

EditorHeader::EditorHeader() {}

std::string
EditorHeader::truncateFilePath(const std::string &path, float maxWidth, ImFont *font)
{
	if (path.empty())
	{
		return "";
	}

	fs::path p(path);
	std::string root_part;
	std::vector<std::string> components;

	// Split into root and components
	if (p.has_root_path())
	{
		root_part = p.root_path().string();
		// Iterate over components after the root
		for (auto it = ++p.begin(); it != p.end(); ++it)
		{
			if (!it->empty())
			{
				components.push_back(it->string());
			}
		}
	} else
	{
		for (const auto &part : p)
		{
			if (!part.empty())
			{
				components.push_back(part.string());
			}
		}
	}

	if (components.empty())
	{
		return root_part.empty() ? path : root_part;
	}

	// Check full path first
	std::string fullPath =
		root_part + std::accumulate(components.begin(),
									components.end(),
									std::string(),
									[](const std::string &a, const std::string &b) {
										return a.empty() ? b : a + "/" + b;
									});
	if (ImGui::CalcTextSize(fullPath.c_str()).x <= maxWidth)
	{
		return fullPath;
	}
	// Try removing directories from the front
	for (size_t start = 0; start < components.size(); ++start)
	{
		std::string candidate;

		if (start == 0)
		{
			candidate = fullPath;
		} else
		{
			std::string middle =
				".../" + std::accumulate(components.begin() + start,
										 components.end(),
										 std::string(),
										 [](const std::string &a, const std::string &b) {
											 return a.empty() ? b : a + "/" + b;
										 });
			candidate = root_part + middle;
		}

		float width = ImGui::CalcTextSize(candidate.c_str()).x;
		if (width <= maxWidth)
		{
			return candidate;
		}
	}

	// Only filename left (with root if applicable)
	std::string filename = root_part + components.back();
	if (ImGui::CalcTextSize(filename.c_str()).x <= maxWidth)
	{
		return filename;
	}

	// Truncate filename
	std::string truncated = components.back();
	int maxLength = truncated.length();
	while (maxLength > 0)
	{
		std::string temp = truncated.substr(0, maxLength) + "...";
		std::string candidate = root_part + temp;
		if (ImGui::CalcTextSize(candidate.c_str()).x <= maxWidth)
		{
			return candidate;
		}
		maxLength--;
	}

	// Minimum case
	return root_part + "...";
}

void EditorHeader::renderSettingsIcon(float iconSize)
{
	bool settingsOpen = gSettings.showSettingsWindow;
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));

	// Vertical centering
	float textHeight = ImGui::GetTextLineHeight();
	float iconTopY = ImGui::GetCursorPosY() + (textHeight - iconSize) * 0.5f - 2.0f;
	ImGui::SetCursorPosY(iconTopY);

	ImVec2 cursor_pos = ImGui::GetCursorPos();
	if (ImGui::InvisibleButton("##gear-hitbox", ImVec2(iconSize, iconSize)))
	{
		gSettings.toggleSettingsWindow();
	}
	bool isHovered = ImGui::IsItemHovered();
	ImGui::SetCursorPos(cursor_pos);
	ImTextureID icon = isHovered ? getStatusIcon("gear-hover") : getStatusIcon("gear");
	ImGui::Image(icon, ImVec2(iconSize, iconSize));

	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar();
}

void EditorHeader::renderTerminalIcon(float iconSize)
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));

	// Vertical centering
	float textHeight = ImGui::GetTextLineHeight();
	float iconTopY = ImGui::GetCursorPosY() + (textHeight - iconSize) * 0.5f;
	ImGui::SetCursorPosY(iconTopY);

	ImVec2 cursor_pos = ImGui::GetCursorPos();
	if (ImGui::InvisibleButton("##terminal-hitbox", ImVec2(iconSize, iconSize)))
	{
		gTerminal.toggleVisibility();
	}
	bool isHovered = ImGui::IsItemHovered();
	ImGui::SetCursorPos(cursor_pos);
	ImTextureID icon =
		isHovered ? getStatusIcon("terminal-hover") : getStatusIcon("terminal");
	ImGui::Image(icon, ImVec2(iconSize, iconSize));

	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar();
}

ImTextureID EditorHeader::getFileIcon(const std::string &filename)
{
	return gFileExplorer.getIconForFile(filename);
}

ImTextureID EditorHeader::getStatusIcon(const std::string &iconName)
{
	return gFileExplorer.getIcon(iconName);
}

void EditorHeader::render(ImFont *font,
						  const std::string &currentFile,
						  bool showGitChanges)
{
	float windowWidth = ImGui::GetWindowWidth();

	ImGui::BeginGroup();
	ImGui::PushFont(font);

	// Determine the base icon size (equal to font size)
	float iconSize = ImGui::GetFontSize() * 1.15f;

	// Calculate space needed for right-aligned status area
	const float rightPadding = 25.0f; // Space from window edge
	const float totalStatusWidth =
		iconSize * 3 + rightPadding; // Brain + Terminal + Gear icons

	// Calculate space needed for git changes if enabled and available
	float gitChangesWidth = 0.0f;
	if (showGitChanges && gSettings.getSettings()["git_changed_lines"] &&
		!gEditorGit.currentGitChanges.empty())
	{
		gitChangesWidth = ImGui::CalcTextSize(gEditorGit.currentGitChanges.c_str()).x +
						  ImGui::GetStyle().ItemSpacing.x;
	}

	// Render left side (file icon and name)
	if (currentFile.empty())
	{
		ImGui::Text("Editor - No file selected");
	} else
	{
		ImTextureID fileIcon = getFileIcon(currentFile);
		if (fileIcon)
		{
			// Vertical centering for file icon
			float textHeight = ImGui::GetTextLineHeight();
			float iconTopY = ImGui::GetCursorPosY() + (textHeight - iconSize) * 0.5f;
			ImGui::SetCursorPosY(iconTopY);
			ImGui::Image(fileIcon, ImVec2(iconSize, iconSize));
			ImGui::SameLine();
		}

		// Calculate available width for the text, accounting for all elements
		float x_cursor = ImGui::GetCursorPosX();
		float x_right_group = ImGui::GetWindowWidth();
		float available_width = x_right_group - x_cursor -
								ImGui::GetStyle().ItemSpacing.x - totalStatusWidth -
								gitChangesWidth;

		// Truncate the file path to fit available space
		std::string truncatedText =
			truncateFilePath(currentFile, available_width, ImGui::GetFont());

		ImGui::Text("%s", truncatedText.c_str());

		// Add git changes info if available and window is wide enough
		if (showGitChanges && gSettings.getSettings()["git_changed_lines"])
		{
			if (!gEditorGit.currentGitChanges.empty())
			{
				ImGui::SameLine();
				ImGui::Text("%s", gEditorGit.currentGitChanges.c_str());
			}
		}
	}

	// Right-aligned status area
	// Position at far right edge
	ImGui::SameLine(ImGui::GetWindowWidth() - totalStatusWidth);

	// Status group
	ImGui::BeginGroup();
	{
		// Vertical centering
		float textHeight = ImGui::GetTextLineHeight();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (textHeight - iconSize) * 0.5f);

		// Brain icon (only visible when active)
		if (gAITab.request_active)
		{
			ImGui::Image(getStatusIcon("green-dot"), ImVec2(iconSize, iconSize));
		} else
		{
			// Invisible placeholder to maintain layout
			ImGui::InvisibleButton("##brain-placeholder", ImVec2(iconSize, iconSize));
		}
		ImGui::SameLine();

		// Terminal icon (newly added)
		renderTerminalIcon(iconSize * 0.7f);
		ImGui::SameLine();

		// Settings icon (always in same position)
		renderSettingsIcon(iconSize * 0.65f);
	}
	ImGui::EndGroup();

	ImGui::PopFont();
	ImGui::EndGroup();
	ImGui::Separator();
}