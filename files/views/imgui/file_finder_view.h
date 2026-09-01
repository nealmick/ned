/*
	File: views/imgui/file_finder_view.h
	Description: ImGui Ctrl+P project file finder popup — window, search
	input, results list. FileFinder in files/ owns scan/filter/select logic.
*/

#pragma once

#include "imgui.h"

class FileFinder;

void renderFileFinder(FileFinder &f);
