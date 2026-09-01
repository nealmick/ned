/*
	File: views/imgui/file_tree_view.h
	Description: ImGui rendering of file-tree rows (recursion, icons, theme
	colors). FileTree in files/ owns the model; this only draws it.
*/

#pragma once

class FileTree;
struct FileNode;

void renderFileTree(FileTree &tree, FileNode &node, int depth = 0);
