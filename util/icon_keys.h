/*
	File: util/icon_keys.h
	Description: Pure filename -> icon-key resolution, shared by both UI
	backends (GL texture atlas in host/imgui; SVG rendering in host/qt).
	Toolkit-neutral — lives in core; backends only render the keys.
*/

#pragma once

#include <string>

// "main.cpp" → "cpp", "CMakeLists.txt" → "cmake", ".gitignore" → "gitignore", ...
std::string iconKeyForFile(const std::string &filename);
