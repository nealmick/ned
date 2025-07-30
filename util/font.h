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
	bool needFontReload;

	Font();
	~Font();

	// Initialize fonts
	void initialize();

	// Handle font reloading
	void handleFontReload();

	// Handle font reloading with frame updates
	void handleFontReloadWithFrameUpdates();

	// Get current font
	ImFont *getCurrentFont() const { return currentFont; }

	// Get font reload state
	bool getNeedFontReload() const { return needFontReload; }
	void setNeedFontReload(bool need) { needFontReload = need; }
};

// Global font instance
extern Font gFont;