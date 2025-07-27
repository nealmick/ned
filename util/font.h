/*
File: font.h
Description: Font management class for NED text editor.
*/

#pragma once

#include <string>

struct ImFont;
struct ImGuiIO;

// Forward declaration
class Frame;

class Font
{
  public:
	// Load main font with emoji support and various character ranges
	static ImFont *loadFont(const std::string &fontName, float fontSize);

	// Load large font for overlays and special displays
	static ImFont *loadLargeFont(const std::string &fontName, float fontSize);

	// Font instances
	ImFont *currentFont;
	ImFont *largeFont;

	Font();
	~Font();

	// Initialize fonts
	void initialize();

	// Handle font reloading
	void handleFontReload(bool &needFontReload);

	// Handle font reloading with frame updates
	void handleFontReloadWithFrameUpdates(bool &needFontReload);

	// Get current font
	ImFont *getCurrentFont() const { return currentFont; }
};

// Global font instance
extern Font gFont;