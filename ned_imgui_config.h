// Custom ImGui configuration for ned editor
// This file is included instead of the default imconfig.h to avoid modifying the submodule

#pragma once

// Use 32-bit for ImWchar (default is 16-bit) to support Unicode planes 1-16
// (e.g. point beyond 0xFFFF like emoticons, dingbats, symbols, shapes, ancient languages,
// etc...)
#define IMGUI_USE_WCHAR32

// Use FreeType to build and rasterize the font atlas (instead of stb_truetype which is
// embedded by default in Dear ImGui) Requires FreeType headers to be available in the
// include path. Requires program to be compiled with 'misc/freetype/imgui_freetype.cpp'
// (in this repository) + the FreeType library (not provided). On Windows you may use
// vcpkg with 'vcpkg install freetype --triplet=x64-windows' + 'vcpkg integrate install'.
#define IMGUI_ENABLE_FREETYPE