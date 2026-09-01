/*
File: font.h
Description: Font management class for NED text editor.
*/

#pragma once

#include "imgui.h"
#include <string>
#include <vector>

class Font
{
  public:
	Font() = default;

	// clearAtlas=false re-adds editor fonts after another system (e.g. terminal)
	// rebuilt the ImGui atlas without wiping its own glyphs.
	void load(bool clearAtlas = true);
	void setFont(const std::string &fontName, float fontSize);

	const std::vector<std::string> &availableFonts() const { return fontNames; }
	const std::string &getFontName() const { return fontName; }
	float getFontSize() const { return fontSize; }

	ImFont *getMainFont() const { return currentFont; }
	ImFont *getLargeFont() const { return largeFont; }

  private:
	ImFont *loadFont(const std::string &fontName, float fontSize);
	ImFont *loadLargeFont(const std::string &fontName, float fontSize);

	std::string fontName;
	float fontSize = 0.0f;

	ImFont *currentFont = nullptr;
	ImFont *largeFont = nullptr;

	std::vector<std::string> fontNames = {
		"SourceCodePro-Regular",
		"JetBrainsMonoNL-Regular",
		"NotoSansMono-Regular",
		"NotoSansMono-Thin",
		"NotoSansMono-Light",
		"VT323-Regular",
		"IBM_MDA",
		"VT100",
	};

	const ImWchar *mainGlyphRanges() const;
	const ImWchar *emojiGlyphRanges() const;
	const ImWchar *brailleGlyphRanges() const;
	const ImWchar *largeFontGlyphRanges() const;

	void mergeBrailleFont(ImGuiIO &io, float fontSize) const;
	void mergeEmojiFont(ImGuiIO &io, float fontSize) const;

	std::string fontResourcePath(const std::string &filename) const;

	void applyMainFontTweaks(ImFontConfig &config, const std::string &fontName) const;
};
