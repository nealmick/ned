#pragma once

#include "../../editor/platform/ned_types.h"
#include "../../util/icon_keys.h"
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
	NedTextureId get(const std::string &name) const;

	// Icon for a file path or bare filename (special names + extension map).
	NedTextureId getForFile(const std::string &filename) const;

	// Pure resolution: filename -> icon key ("cpp", "cmake", ...). Backends
	// without the GL texture atlas (Qt) use the free iconKeyForFile() +
	// their own renderer.
	static std::string iconKeyForFile(const std::string &filename)
	{
		return ::iconKeyForFile(filename);
	}

  private:
	static constexpr int ICON_SIZE = 32;
	static constexpr float SVG_DPI = 96.0f;

	std::map<std::string, NedTextureId> textures;

	uint32_t createTexture(const unsigned char *pixels, int width, int height);
	bool loadSvg(const std::string &iconFile);
	void createFallback();
};
