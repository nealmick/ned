/*
	File: views/qt/qt_sidebar.h
	Description: File-tree sidebar for the Qt backend, rendered over the
	shared FileTree model (files/file_tree.h). Lazily expands directories
	by delegating to FileTree::buildFileTree.
*/

#pragma once

#include <QTreeWidget>

#include "../../../files/file_tree.h"

class QtFileSidebar : public QTreeWidget
{
	Q_OBJECT

  public:
	explicit QtFileSidebar(QWidget *parent = nullptr);

	void openWorkspace(const QString &root);
	QString workspace() const { return rootPath; }

  Q_SIGNALS:
	void fileActivated(const QString &path);

  private:
	void populate(QTreeWidgetItem *parentItem, FileNode &node);
	void rescan();

	QString rootPath;
	FileTree tree;
};
