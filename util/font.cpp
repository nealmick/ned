/*
File: font.cpp
Description: Font management class implementation for NED text editor.
*/

#include "font.h"
#include "imgui.h"
#include "settings.h"
#include <filesystem>
#include <iostream>

#ifdef IMGUI_ENABLE_FREETYPE
#include "misc/freetype/imgui_freetype.h"
#endif

std::string Font::fontResourcePath(const std::string &filename) const
{
	return Settings::getAppResourcesPath() + "/resources/fonts/" + filename;
}

const ImWchar *Font::mainGlyphRanges() const
{
	static const ImWchar ranges[] = {
		0x0020,	 0x00FF,  0x2500,  0x257F,	0x2580,	 0x259F,  0x25A0,  0x25FF,	0x2600,
		0x26FF,	 0x2700,  0x27BF,  0x2900,	0x297F,	 0x2B00,  0x2BFF,  0x3000,	0x303F,
		0xE000,	 0xE0FF,  0x1F300, 0x1F9FF, 0x1F600, 0x1F64F, 0x1F680, 0x1F6FF, 0x1F900,
		0x1F9FF, 0xFE00,  0xFE0F,  0x1F000, 0x1F02F, 0x1F0A0, 0x1F0FF, 0x1F100, 0x1F64F,
		0x1F650, 0x1F67F, 0x1F700, 0x1F77F, 0x1F780, 0x1F7FF, 0x1F800, 0x1F8FF, 0x1FA00,
		0x1FA6F, 0x1FA70, 0x1FAFF, 0x1FB00, 0x1FBFF, 0,
	};
	return ranges;
}

const ImWchar *Font::emojiGlyphRanges() const
{
	static const ImWchar ranges[] = {
		0x2600,	 0x26FF,  0x2700,  0x27BF,	0xFE00,	 0xFE0F,  0x1F000, 0x1F02F,
		0x1F0A0, 0x1F0FF, 0x1F100, 0x1F64F, 0x1F650, 0x1F67F, 0x1F680, 0x1F6FF,
		0x1F700, 0x1F77F, 0x1F780, 0x1F7FF, 0x1F800, 0x1F8FF, 0x1F900, 0x1F9FF,
		0x1FA00, 0x1FA6F, 0x1FA70, 0x1FAFF, 0x1FB00, 0x1FBFF, 0,
	};
	return ranges;
}

const ImWchar *Font::brailleGlyphRanges() const
{
	static const ImWchar ranges[] = {0x2800, 0x28FF, 0};
	return ranges;
}

const ImWchar *Font::largeFontGlyphRanges() const
{
	static const ImWchar ranges[] = {0x0020, 0x00FF, 0};
	return ranges;
}

void Font::applyMainFontTweaks(ImFontConfig &config, const std::string &fontName) const
{
	if (fontName.find("vt100") != std::string::npos ||
		fontName.find("VT100") != std::string::npos)
	{
		config.GlyphOffset.x = 3.0f;
		config.PixelSnapH = true;
	}
}

void Font::mergeBrailleFont(ImGuiIO &io, float fontSize) const
{
	const std::string path = fontResourcePath("DejaVuSans.ttf");
	if (!std::filesystem::exists(path))
	{
		return;
	}

	ImFontConfig config;
	config.MergeMode = true;
	io.Fonts->AddFontFromFileTTF(path.c_str(), fontSize, &config, brailleGlyphRanges());
}

void Font::mergeEmojiFont(ImGuiIO &io, float fontSize) const
{
	std::string path = fontResourcePath("seguiemj.ttf");
	if (std::filesystem::exists(path))
	{
		ImFontConfig config;
		config.MergeMode = true;
#ifdef IMGUI_ENABLE_FREETYPE
		config.FontLoaderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;
#endif
		io.Fonts->AddFontFromFileTTF(
			path.c_str(), fontSize * 0.7f, &config, emojiGlyphRanges());
		return;
	}

	path = fontResourcePath("Emoji.ttf");
	if (std::filesystem::exists(path))
	{
		ImFontConfig config;
		config.MergeMode = true;
#ifdef IMGUI_ENABLE_FREETYPE
		config.FontLoaderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;
#endif
		io.Fonts->AddFontFromFileTTF(
			path.c_str(), fontSize * 0.7f, &config, emojiGlyphRanges());
	}
}

ImFont *Font::loadFont(const std::string &fontName, float fontSize)
{
	ImGuiIO &io = ImGui::GetIO();
	const std::string path = fontResourcePath(fontName + ".ttf");

	if (!std::filesystem::exists(path))
	{
		std::cerr << "[Font] Font does not exist: " << path << std::endl;
		return nullptr;
	}

	ImFontConfig config;
	config.MergeMode = false;
	config.GlyphRanges = mainGlyphRanges();
	applyMainFontTweaks(config, fontName);

	ImFont *font =
		io.Fonts->AddFontFromFileTTF(path.c_str(), fontSize, &config, mainGlyphRanges());
	if (!font)
	{
		std::cerr << "[Font] Failed to load font: " << path << std::endl;
		return nullptr;
	}

	mergeBrailleFont(io, fontSize);
	mergeEmojiFont(io, fontSize);
	return font;
}

ImFont *Font::loadLargeFont(const std::string &fontName, float fontSize)
{
	ImGuiIO &io = ImGui::GetIO();
	const std::string path = fontResourcePath(fontName + ".ttf");

	if (!std::filesystem::exists(path))
	{
		std::cerr << "[Font] Large font does not exist: " << path << std::endl;
		return nullptr;
	}

	ImFontConfig config;
	config.MergeMode = false;
	config.GlyphRanges = largeFontGlyphRanges();

	ImFont *font = io.Fonts->AddFontFromFileTTF(
		path.c_str(), fontSize, &config, largeFontGlyphRanges());
	if (font == nullptr)
	{
		std::cerr << "[Font] Failed to load large font: " << path << std::endl;
	}
	return font;
}

void Font::setFont(const std::string &fontName, float fontSize)
{
	this->fontName = fontName;
	this->fontSize = fontSize;
}

void Font::load(bool clearAtlas)
{
	if (clearAtlas)
		ImGui::GetIO().Fonts->Clear();

	currentFont = loadFont(getFontName(), getFontSize());
	largeFont = loadLargeFont(getFontName(), 52.0f);

	if (!currentFont)
	{
		std::cerr << "[Font] Failed to load font, using default font" << std::endl;
		currentFont = ImGui::GetIO().Fonts->AddFontDefault();
	}

	if (!largeFont)
	{
		std::cerr << "[Font] Failed to load large font, using default font" << std::endl;
		largeFont = ImGui::GetIO().Fonts->AddFontDefault();
	}

	ImGui::GetIO().Fonts->Build();
}
