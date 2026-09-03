#include "file_tree_view.h"

#include "files/file_tree.h"
#include "util/qt_icons.h"

#include <QString>
#include <QTreeWidget>

#include <algorithm>

FileNode *fileTreeNodeOf(QTreeWidgetItem *item)
{
	return static_cast<FileNode *>(item->data(0, Qt::UserRole).value<void *>());
}

void populateFileTree(QTreeWidgetItem *parentItem, FileNode &node, int iconPx)
{
	for (FileNode &child : node.children)
	{
		auto *item = new QTreeWidgetItem({QString::fromStdString(child.name)});
		item->setData(0, Qt::UserRole, QVariant::fromValue<void *>(&child));
		applyFileTreeRowIcon(item, child, iconPx);
		parentItem->addChild(item);
		if (child.isDirectory)
		{
			if (!child.children.empty())
				populateFileTree(item, child, iconPx);
			item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
		}
	}
}

void applyFileTreeRowIcon(QTreeWidgetItem *item, const FileNode &node, int iconPx)
{
	// File-type SVGs carry intrinsic padding (smaller glyph in the same
	// viewBox) — rasterize them a notch larger than folders.
	const int px = std::max(10, iconPx);
	item->setIcon(0,
				  node.isDirectory
					  ? QtIconSet::byKey(node.isOpen ? "folder-open" : "folder", px)
					  : QtIconSet::forFile(QString::fromStdString(node.name),
										   std::max(12, px * 3 / 2)));
}
