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
#include <algorithm>

// External dependencies
extern class FileExplorer gFileExplorer;
extern class Settings gSettings;
extern class AITab gAITab;
extern class EditorGit gEditorGit;

namespace fs = std::filesystem;

EditorHeader::EditorHeader() {}

// Helper function to convert forward slashes to backslashes on Windows
std::string EditorHeader::normalizePathForDisplay(const std::string &path)
{
#ifdef PLATFORM_WINDOWS
	std::string result = path;
	// Convert forward slashes to backslashes
	std::replace(result.begin(), result.end(), '/', '\\');

	// Remove consecutive backslashes (but preserve UNC paths that start with \\)
	std::string cleaned;
	bool lastWasBackslash = false;
	bool isUNCPath = result.length() >= 2 && result[0] == '\\' && result[1] == '\\';

	for (size_t i = 0; i < result.length(); ++i)
	{
		char c = result[i];
		if (c == '\\')
		{
			if (!lastWasBackslash || (i == 1 && isUNCPath))
			{
				// Add the backslash if it's not consecutive, or if it's the second slash
				// in UNC path
				cleaned += c;
				lastWasBackslash = true;
			}
			// Skip consecutive backslashes
		} else
		{
			cleaned += c;
			lastWasBackslash = false;
		}
	}

	return cleaned;
#else
	return path;
#endif
}

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
		return normalizePathForDisplay(root_part.empty() ? path : root_part);
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
		return normalizePathForDisplay(fullPath);
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
			return normalizePathForDisplay(candidate);
		}
	}

	// Only filename left (with root if applicable)
	std::string filename = root_part + components.back();
	if (ImGui::CalcTextSize(filename.c_str()).x <= maxWidth)
	{
		return normalizePathForDisplay(filename);
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
			return normalizePathForDisplay(candidate);
		}
		maxLength--;
	}

	// Minimum case
	return normalizePathForDisplay(root_part + "...");
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
	const float rightPadding =
		(currentFile == "Terminal") ? 25.0f : 25.0f; // Same padding as normal
#ifdef PLATFORM_WINDOWS
	const float totalStatusWidth =
		iconSize * 2 + rightPadding; // Brain + Gear icons (no terminal on Windows)
#else
	const float totalStatusWidth =
		iconSize * 3 + rightPadding; // Brain + Terminal + Gear icons
#endif

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
		// Special case: show shell icon for Terminal
		ImTextureID fileIcon;
		if (currentFile == "Terminal")
		{
			fileIcon = getStatusIcon("sh");
		} else
		{
			fileIcon = getFileIcon(currentFile);
		}
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
		// Add right margin for terminal
		if (currentFile == "Terminal")
		{
			x_right_group -= 10.0f; // 10px right margin for terminal
		}
		float available_width = x_right_group - x_cursor -
								ImGui::GetStyle().ItemSpacing.x - totalStatusWidth -
								gitChangesWidth;

		// Truncate the file path to fit available space
		std::string truncatedText =
			truncateFilePath(currentFile, available_width, ImGui::GetFont());

		// Special positioning for Terminal text
		if (currentFile == "Terminal")
		{
			ImVec2 currentPos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(currentPos.x - 7.0f, currentPos.y + 3.0f));
		}

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
	// Position at far right edge (with margin for terminal)
	float rightEdge = ImGui::GetWindowWidth() - totalStatusWidth;
	if (currentFile == "Terminal")
	{
		rightEdge -= 20.0f; // 20px right margin for terminal
	}
	ImGui::SameLine(rightEdge);

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

#ifndef PLATFORM_WINDOWS
		// Terminal icon (newly added)
		renderTerminalIcon(iconSize * 0.7f);
		ImGui::SameLine();
#endif

		// Settings icon (always in same position)
		renderSettingsIcon(iconSize * 0.65f);
	}
	ImGui::EndGroup();

	ImGui::PopFont();
	ImGui::EndGroup();

	// Custom separator for terminal to respect margins
	if (currentFile == "Terminal")
	{
		ImDrawList *draw_list = ImGui::GetWindowDrawList();
		ImVec2 p = ImGui::GetCursorScreenPos();
		// Equal 19px margin on both sides
		float windowLeft = ImGui::GetWindowPos().x + 19.0f; // 19px from left edge
		float width =
			ImGui::GetWindowWidth() - 38.0f; // Total width minus 19px on each side
		ImU32 col = ImGui::GetColorU32(ImGuiCol_Separator);
		draw_list->AddLine(ImVec2(windowLeft, p.y), ImVec2(windowLeft + width, p.y), col);
		ImGui::Dummy(ImVec2(0.0f, 1.0f)); // Add separator height
	} else
	{
		ImGui::Separator();
	}
}