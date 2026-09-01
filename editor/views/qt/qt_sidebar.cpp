#include "qt_sidebar.h"

#include "qt_icons.h"
#include <QFileInfo>

QtFileSidebar::QtFileSidebar(QWidget *parent) : QTreeWidget(parent)
{
	setHeaderHidden(true);
	setColumnCount(1);

	connect(this, &QTreeWidget::itemExpanded, this, [this](QTreeWidgetItem *item) {
		// Lazy expansion: build children on first open, like the ImGui tree.
		FileNode *node =
			static_cast<FileNode *>(item->data(0, Qt::UserRole).value<void *>());
		if (node && !node->isOpen && node->isDirectory)
		{
			node->isOpen = true;
			tree.buildFileTree(node->fullPath, *node);
			item->takeChildren();
			populate(item, *node);
		}
	});

	connect(this, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int) {
		FileNode *node =
			static_cast<FileNode *>(item->data(0, Qt::UserRole).value<void *>());
		if (node && !node->isDirectory)
			Q_EMIT fileActivated(QString::fromStdString(node->fullPath));
	});
}

void QtFileSidebar::openWorkspace(const QString &root)
{
	rootPath = root;
	rescan();
}

void QtFileSidebar::rescan()
{
	clear();
	if (rootPath.isEmpty())
		return;

	tree.rootNode = FileNode{};
	tree.rootNode.name = QFileInfo(rootPath).fileName().toStdString();
	tree.rootNode.fullPath = rootPath.toStdString();
	tree.rootNode.isDirectory = true;
	tree.rootNode.isOpen = true;
	tree.buildFileTree(tree.rootNode.fullPath, tree.rootNode);

	// Workspace children at top level — no wrapper node for the root
	// folder itself (ImGui tree parity).
	populate(invisibleRootItem(), tree.rootNode);
}

void QtFileSidebar::populate(QTreeWidgetItem *parentItem, FileNode &node)
{
	for (FileNode &child : node.children)
	{
		auto *item = new QTreeWidgetItem({QString::fromStdString(child.name)});
		item->setIcon(
			0,
			child.isDirectory
				? QtIconSet::instance().byKey(child.isOpen ? "folder-open" : "folder")
				: QtIconSet::instance().forFile(QString::fromStdString(child.name)));
		item->setData(0, Qt::UserRole, QVariant::fromValue<void *>(&child));
		parentItem->addChild(item);
		if (child.isDirectory)
		{
			if (!child.children.empty())
				populate(item, child);
			item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
		}
	}
}
