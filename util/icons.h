#pragma once

#include "imgui.h"
#include <cstdint>
#include <map>
#include <string>

// Loads SVG icons from resources/icons/ into OpenGL textures and looks them up
// by name ("folder") or by file path/extension ("main.cpp" → "cpp").
class Icons
{
  public:
	// Rasterize every known icon SVG into a texture. Safe to call once after GL is ready.
	void load();

	// Lookup by icon key (filename without extension), e.g. "folder", "gear-hover".
	ImTextureID get(const std::string &name) const;

	// Icon for a file path or bare filename (special names + extension map).
	ImTextureID getForFile(const std::string &filename) const;

  private:
	static constexpr int ICON_SIZE = 32;
	static constexpr float SVG_DPI = 96.0f;

	std::map<std::string, ImTextureID> textures;

	uint32_t createTexture(const unsigned char *pixels, int width, int height);
	bool loadSvg(const std::string &iconFile);
	void createFallback();
};
