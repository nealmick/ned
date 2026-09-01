/*
	File: views/imgui/file_sidebar_view.h
	Description: ImGui file-explorer sidebar (tree panel + optional toolbar
	when the host has no native title bar).
*/

#pragma once

class FileExplorer;

void renderFileSidebar(FileExplorer &fx, float explorerWidth);
