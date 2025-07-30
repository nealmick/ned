/*
File: editor_header.h
Description: Editor header rendering class for NED text editor.
*/

#pragma once

#include "imgui.h"
#include <filesystem>
#include <string>

// Forward declarations
struct ImFont;

class EditorHeader
{
  public:
	EditorHeader();
	~EditorHeader() = default;

	// Main rendering function
	void render(ImFont *font, const std::string &currentFile, bool showGitChanges);

  private:
	// File path truncation utility
	std::string truncateFilePath(const std::string &path, float maxWidth, ImFont *font);

	// Settings icon rendering
	void renderSettingsIcon(float iconSize);

	// Helper function to get file icon
	ImTextureID getFileIcon(const std::string &filename);

	// Helper function to get status icons
	ImTextureID getStatusIcon(const std::string &iconName);
};