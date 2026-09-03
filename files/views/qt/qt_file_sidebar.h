/*
	File: files/views/qt/qt_file_sidebar.h
	Description: File-tree sidebar for the Qt backend (parallel of
	files/views/imgui/file_sidebar_view), rendered over the shared
	FileTree model (files/file_tree.h). Lazily expands directories
	by delegating to FileTree::buildFileTree.
*/

#pragma once

#include <QPersistentModelIndex>
#include <QTreeWidget>

#include "files/file_tree.h"

class QtFileSidebar : public QTreeWidget
{
	Q_OBJECT

  public:
	explicit QtFileSidebar(QWidget *parent = nullptr);

	void openWorkspace(const QString &root);
	// Icon size tracks the app font (called on font changes).
	void refreshIconScale();
	QString workspace() const { return rootPath; }

  Q_SIGNALS:
	void fileActivated(const QString &path);

  protected:
	// Row highlight (hover + selection) is painted here as ONE rounded pill
	// per full row. The tree runs NoSelection (see .cpp) so Qt never paints
	// its own — per-cell stylesheet boxes read as two separate buttons
	// (arrow | label) once the corners are rounded.
	void paintEvent(QPaintEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void leaveEvent(QEvent *event) override;

  private:
	void populate(QTreeWidgetItem *parentItem, FileNode &node);
	// Icon size from the app font (rows created after this read it).
	void updateIconSize();
	// Folder rows swap folder/folder-open with expansion state (ImGui tree
	// parity); file rows pick the icon by extension.
	void applyRowIcon(QTreeWidgetItem *item, const FileNode &node);
	void rescan();
	void setHover(const QModelIndex &index);

	QString rootPath;
	FileTree tree;
	QPersistentModelIndex hoverIndex;
};
