/*
File: font.h
Description: Font management class for loading and managing ImGui fonts.
*/

#pragma once

#include "imgui.h"
#include <string>

class Font
{
  public:
	Font();
	~Font();

	// Initialize fonts
	void initialize();

	// Load main font with full Unicode support and emoji
	ImFont *loadFont(const std::string &fontName, float fontSize);

	// Load large font for overlays (simplified Unicode support)
	ImFont *loadLargeFont(const std::string &fontName, float fontSize);

	// Handle font reloading when settings change
	void handleFontReload(bool &needFontReload);

	// Getter methods for accessing fonts
	ImFont *getCurrentFont() const { return currentFont; }
	ImFont *getLargeFont() const { return largeFont; }

  private:
	// Helper function to rebuild font textures
	void rebuildFontTextures();

	// Font instances
	ImFont *currentFont;
	ImFont *largeFont;
};