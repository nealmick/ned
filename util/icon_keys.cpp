/*
	File: util/icon_keys.cpp
	Description: Pure filename -> icon-key resolution, shared by both UI
	backends (GL texture atlas in icons.cpp; SVG rendering in views/qt).
	Kept out of icons.cpp so backends without the GL atlas can link it.
*/

#include "imgui_icons.h"

#include <filesystem>

namespace fs = std::filesystem;

std::string Icons::iconKeyForFile(const std::string &filename)
{
	const std::string name = fs::path(filename).filename().string();

	if (name == "CMakeLists.txt" || name == "cmake")
		return "cmake";
	if (name == ".clangd" || name == ".clang-format")
		return "clangd";
	if (name == "Dockerfile")
		return "Dockerfile";
	if (name == ".gitignore")
		return "gitignore";
	if (name == ".gitmodules")
		return "gitmodule";

	std::string ext = fs::path(filename).extension().string();
	if (!ext.empty() && ext[0] == '.')
		ext.erase(0, 1);

	return ext.empty() ? "default" : ext;
}
