/*
	File: files/views/qt/file_tree_view.h
	Description: Qt rendering of file-tree rows (population, expansion
	icons) over the shared FileTree model (files/file_tree.h) — parallel
	of files/views/imgui/file_tree_view.
*/

#pragma once

class QTreeWidgetItem;

struct FileNode;

// Rows carry their FileNode as opaque user data (pointers into the
// shared tree — stable because populate/lazy-expand only APPEND to a
// node's children and rescan clears items before resetting the tree).
FileNode *fileTreeNodeOf(QTreeWidgetItem *item);

// Recursive row creation from a model node. iconPx is the current app
// icon size (rows created after a font change read it from the caller).
void populateFileTree(QTreeWidgetItem *parentItem, FileNode &node, int iconPx);

// Folder rows swap folder/folder-open with expansion state (ImGui tree
// parity); file rows pick the icon by extension.
void applyFileTreeRowIcon(QTreeWidgetItem *item, const FileNode &node, int iconPx);
