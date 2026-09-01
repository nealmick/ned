/*
	File: views/imgui/file_views.h
	Description: ImGui rendering entry points for the files module.
	The files/ module owns state and logic; these draw it.
*/

#pragma once

class FileExplorer;
class FileFinder;
class FileTree;
struct FileNode;

// Sidebar: file tree panel (+ optional toolbar when no native title bar).
void renderFileSidebar(FileExplorer &fx, float explorerWidth);

// Recursive file-tree rows.
void renderFileTree(FileTree &tree, FileNode &node, int depth = 0);

// Ctrl+P project file finder popup (window, input, results list).
// The helper pieces are declared here so FileFinder can befriend them.
class FileFinder;
void renderFileFinder(FileFinder &f);
void renderFileFinderHeader(FileFinder &f);
bool renderFileFinderSearchInput(FileFinder &f);
void renderFileFinderList(FileFinder &f);
