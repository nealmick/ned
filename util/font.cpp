/*
File: font.cpp
Description: Font management class implementation for NED text editor.
*/

#include "font.h"
// Frame functionality moved to Render class
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "settings.h"
#include "terminal.h"
#include <filesystem>
#include <iostream>

#ifdef IMGUI_ENABLE_FREETYPE
#include "misc/freetype/imgui_freetype.h"
#endif

// Global font instance
extern Font gFont;

ImFont *Font::loadFont(const std::string &fontName, float fontSize)
{
	ImGuiIO &io = ImGui::GetIO();

	// Build the path from .app/Contents/Resources/fonts/
	std::string resourcePath = Settings::getAppResourcesPath();
	std::string fontPath = resourcePath + "/fonts/" + fontName + ".ttf";
	// Always print the path, before existence check
	// std::cout << "[Font::loadFont] Attempting to load font from: " << fontPath
	// << " at size "
	//		  << fontSize << std::endl;

	if (!std::filesystem::exists(fontPath))
	{
		std::cerr << "[Font::loadFont] Font does not exist: " << fontPath << std::endl;
		return io.Fonts->AddFontDefault();
	}

	static const ImWchar ranges[] = {
		0x0020,
		0x00FF, // Basic Latin + Latin Supplement
		0x2500,
		0x257F, // Box Drawing Characters
		0x2580,
		0x259F, // Block Elements
		0x25A0,
		0x25FF, // Geometric Shapes
		0x2600,
		0x26FF, // Miscellaneous Symbols
		0x2700,
		0x27BF, // Dingbats
		0x2900,
		0x297F, // Supplemental Arrows-B
		0x2B00,
		0x2BFF, // Miscellaneous Symbols and Arrows
		0x3000,
		0x303F, // CJK Symbols and Punctuation
		0xE000,
		0xE0FF, // Private Use Area
		// Add emoji ranges to main font as fallback
		0x1F300,
		0x1F9FF, // Miscellaneous Symbols and Pictographs
		0x1F600,
		0x1F64F, // Emoticons
		0x1F680,
		0x1F6FF, // Transport and Map Symbols
		0x1F900,
		0x1F9FF, // Supplemental Symbols and Pictographs
		0xFE00,
		0xFE0F, // Variation Selectors
		0x1F000,
		0x1F02F, // Mahjong Tiles
		0x1F0A0,
		0x1F0FF, // Playing Cards
		0x1F100,
		0x1F64F, // Enclosed Alphanumeric Supplement
		0x1F650,
		0x1F67F, // Ornamental Dingbats
		0x1F700,
		0x1F77F, // Alchemical Symbols
		0x1F780,
		0x1F7FF, // Geometric Shapes Extended
		0x1F800,
		0x1F8FF, // Supplemental Arrows-C
		0x1FA00,
		0x1FA6F, // Chess Symbols
		0x1FA70,
		0x1FAFF, // Symbols and Pictographs Extended-A
		0x1FB00,
		0x1FBFF, // Symbols for Legacy Computing
		0,
	};
	ImFontConfig config_main;
	config_main.MergeMode = false;
	config_main.GlyphRanges = ranges;

	// Font-specific adjustments for alignment issues
	if (fontName.find("vt100") != std::string::npos ||
		fontName.find("VT100") != std::string::npos)
	{
		// VT100 font has character alignment issues - adjust glyph offset
		config_main.GlyphOffset.x = 3.0f; // Shift characters slightly right
		config_main.PixelSnapH = true;	  // Ensure pixel-perfect horizontal alignment
	}

	// Clear existing fonts if you want a single font each time, or
	// if you want to stack multiple fonts, you might skip clearing.
	io.Fonts->Clear();

	ImFont *font =
		io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, &config_main, ranges);

	// Merge DejaVu Sans for Braille, etc. if you want
	ImFontConfig config_braille;
	config_braille.MergeMode = true;
	static const ImWchar braille_ranges[] = {0x2800, 0x28FF, 0};
	std::string dejaVuPath = resourcePath + "/fonts/DejaVuSans.ttf";
	if (std::filesystem::exists(dejaVuPath))
	{
		io.Fonts->AddFontFromFileTTF(
			dejaVuPath.c_str(), fontSize, &config_braille, braille_ranges);
	}

	// Merge emoji font for emoji support
	ImFontConfig config_emoji;
	config_emoji.MergeMode = true;

	// Enable color support for emojis (requires FreeType)
#ifdef IMGUI_ENABLE_FREETYPE
	config_emoji.FontLoaderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;
#endif

	// Try seguiemj.ttf first (more compatible), then Emoji.ttf as fallback
	std::string emojiPath = resourcePath + "/fonts/seguiemj.ttf";
	if (!std::filesystem::exists(emojiPath))
	{
		emojiPath = resourcePath + "/fonts/Emoji.ttf";
	}

	if (std::filesystem::exists(emojiPath))
	{
		// Emoji ranges for merging
		static const ImWchar emoji_ranges[] = {
			0x1F300, 0x1F9FF, // Miscellaneous Symbols and Pictographs
			0x1F600, 0x1F64F, // Emoticons
			0x1F680, 0x1F6FF, // Transport and Map Symbols
			0x1F900, 0x1F9FF, // Supplemental Symbols and Pictographs
			0x2600,	 0x26FF,  // Miscellaneous Symbols
			0x2700,	 0x27BF,  // Dingbats
			0xFE00,	 0xFE0F,  // Variation Selectors
			0x1F000, 0x1F02F, // Mahjong Tiles
			0x1F0A0, 0x1F0FF, // Playing Cards
			0x1F100, 0x1F64F, // Enclosed Alphanumeric Supplement
			0x1F650, 0x1F67F, // Ornamental Dingbats
			0x1F680, 0x1F6FF, // Transport and Map Symbols
			0x1F700, 0x1F77F, // Alchemical Symbols
			0x1F780, 0x1F7FF, // Geometric Shapes Extended
			0x1F800, 0x1F8FF, // Supplemental Arrows-C
			0x1F900, 0x1F9FF, // Supplemental Symbols and Pictographs
			0x1FA00, 0x1FA6F, // Chess Symbols
			0x1FA70, 0x1FAFF, // Symbols and Pictographs Extended-A
			0x1FB00, 0x1FBFF, // Symbols for Legacy Computing
			0,
		};

		// Use a smaller size for emoji font to fit better in terminal
		float emojiFontSize = fontSize * 0.7f;
		io.Fonts->AddFontFromFileTTF(
			emojiPath.c_str(), emojiFontSize, &config_emoji, emoji_ranges);
		std::cout << "[Font::loadFont] Successfully merged emoji font: " << emojiPath
				  << " at size " << emojiFontSize << std::endl;
	} else
	{
		std::cerr << "[Font::loadFont] No emoji font found at: " << emojiPath
				  << std::endl;
	}

	if (!font)
	{
		std::cerr << "[Font::loadFont] Failed to load font: " << fontPath << std::endl;
		return io.Fonts->AddFontDefault();
	}

	// Don't recreate OpenGL texture here - that should only happen in handleFontReload()
	// The texture will be recreated when the font atlas is rebuilt

	// std::cout << "[Font::loadFont] Successfully loaded font: " << fontName <<
	// " from " << fontPath
	//		  << " at size " << fontSize << std::endl;

	return font;
}

ImFont *Font::loadLargeFont(const std::string &fontName, float fontSize)
{
	ImGuiIO &io = ImGui::GetIO();

	// Build the path from .app/Contents/Resources/fonts/
	std::string resourcePath = Settings::getAppResourcesPath();
	std::string fontPath = resourcePath + "/fonts/" + fontName + ".ttf";

	if (!std::filesystem::exists(fontPath))
	{
		std::cerr << "[Font::loadLargeFont] Font does not exist: " << fontPath
				  << std::endl;
		return io.Fonts->AddFontDefault();
	}

	static const ImWchar ranges[] = {
		0x0020,
		0x00FF, // Basic Latin + Latin Supplement
		0,
	};

	ImFontConfig config;
	config.MergeMode = false;
	config.GlyphRanges = ranges;

	// Don't clear existing fonts - just add the new one
	ImFont *font =
		io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, &config, ranges);

	if (!font)
	{
		std::cerr << "[Font::loadLargeFont] Failed to load font: " << fontPath
				  << std::endl;
		return io.Fonts->AddFontDefault();
	}

	// Don't recreate OpenGL texture here - that should only happen in handleFontReload()
	// The texture will be recreated when the font atlas is rebuilt

	return font;
}

// Font class member functions
Font::Font() : currentFont(nullptr), largeFont(nullptr), needFontReload(false) {}

Font::~Font() {}

void Font::initialize()
{
	// Save original font size
	float originalFontSize = gSettings.getSettings()["fontSize"].get<float>();

	// Temporarily force font size to 19.0 for proper terminal initialization
	gSettings.getSettings()["fontSize"] = 19.0f;

	// Load fonts with temporary size
	currentFont = loadFont(gSettings.getCurrentFont(), 19.0f);

	// Restore original font size
	gSettings.getSettings()["fontSize"] = originalFontSize;

	// Load large font for resolution overlay
	largeFont = loadLargeFont(gSettings.getCurrentFont(), 52.0f);

	if (!currentFont)
	{
		std::cerr << "🔴 Failed to load font, using default font" << std::endl;
		currentFont = ImGui::GetIO().Fonts->AddFontDefault();
	}

	if (!largeFont)
	{
		std::cerr << "🔴 Failed to load large font, using default font" << std::endl;
		largeFont = ImGui::GetIO().Fonts->AddFontDefault();
	}

	// Now reload with the correct font size
	needFontReload = true;
	handleFontReload();
}

void Font::handleFontReloadWithFrameUpdates()
{
	// Handle font reloading with frame updates
	if (needFontReload)
	{
		// Frame management is now handled by Render class
		// The calling code should handle frame updates
		handleFontReload();
	}
}

void Font::handleFontReload()
{
	if (needFontReload)
	{
		// Clear the font atlas first
		ImGui::GetIO().Fonts->Clear();

		// Load the new fonts
		currentFont = loadFont(gSettings.getCurrentFont(),
							   gSettings.getSettings()["fontSize"].get<float>());

		// Also reload largeFont since it was cleared above
		largeFont = loadLargeFont(gSettings.getCurrentFont(), 52.0f);

		// Build the font atlas
		ImGui::GetIO().Fonts->Build();

		// For embedded mode, we need to recreate the OpenGL texture
		// but we'll do it more carefully to avoid crashes
		// The texture will be recreated automatically when the font atlas is rebuilt

		// **changed**: Reset terminal font size detection so it will resize on next render
		extern Terminal gTerminal;
		gTerminal.resetFontSizeDetection();

		gSettings.resetFontChanged();
		gSettings.resetFontSizeChanged();
		needFontReload = false;
	}
}