#include "qt_file_sidebar.h"

#include "util/qt_icons.h"

namespace {
// Rows carry their FileNode as opaque user data (pointers into the
// shared tree — stable because populate/lazy-expand only APPEND to a
// node's children and rescan clears items before resetting the tree).
FileNode *nodeOf(QTreeWidgetItem *item)
{
	return static_cast<FileNode *>(item->data(0, Qt::UserRole).value<void *>());
}
} // namespace
#include <QCursor>
#include <QEvent>
#include <QFileInfo>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <algorithm>
#include <functional>

QtFileSidebar::QtFileSidebar(QWidget *parent) : QTreeWidget(parent)
{
	setHeaderHidden(true);
	setColumnCount(1);
	// ImGui parity: no visible scrollbars.
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	// Pixel scrolling like the editor: the default item mode jumps a full
	// row per wheel step, which reads as jittery on trackpads.
	setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	setUniformRowHeights(true);
	// No Qt selection: drawRow() reads the selection MODEL directly and
	// paints the opaque palette highlight over a selected row's branch
	// area — unfixable from a stylesheet or drawRow override. With no
	// selection at all, the rounded pill painted in paintEvent (from
	// currentIndex, which clicks and arrow keys still move) is the only
	// selection visual.
	setSelectionMode(QAbstractItemView::NoSelection);
	// Rows slide under a stationary cursor while wheel-scrolling — the
	// hover pill has to follow the row under the pointer, not the mouse.
	connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this] {
		setHover(viewport()->underMouse()
					 ? indexAt(viewport()->mapFromGlobal(QCursor::pos()))
					 : QModelIndex());
	});

	connect(this, &QTreeWidget::itemExpanded, this, [this](QTreeWidgetItem *item) {
		// Lazy expansion: build children on first open, like the ImGui tree.
		FileNode *node = nodeOf(item);
		if (node && node->isDirectory)
		{
			if (!node->isOpen)
			{
				node->isOpen = true;
				tree.buildFileTree(node->fullPath, *node);
				item->takeChildren();
				populate(item, *node);
			}
			applyRowIcon(item, *node);
		}
	});

	connect(this, &QTreeWidget::itemCollapsed, this, [this](QTreeWidgetItem *item) {
		FileNode *node = nodeOf(item);
		if (node && node->isDirectory)
		{
			node->isOpen = false;
			applyRowIcon(item, *node);
		}
	});

	connect(this, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int) {
		FileNode *node = nodeOf(item);
		if (node && !node->isDirectory)
			Q_EMIT fileActivated(QString::fromStdString(node->fullPath));
	});
}

void QtFileSidebar::paintEvent(QPaintEvent *event)
{
	// Row highlight goes UNDER the base class's paint so text, icons, and
	// branch arrows land on top of it. One rounded pill spans the full row
	// width — stylesheet per-cell boxes made arrow + label look like two
	// buttons once the corners were rounded.
	{
		QPainter painter(viewport());
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setPen(Qt::NoPen);
		const auto paintRow = [&](const QModelIndex &index, const QColor &fill) {
			const QRect row = visualRect(index);
			if (row.isNull())
				return;
			// Match the accent the QListWidget rules use (169,174,182 at
			// 0.32) and the 6px radius of the tab/input language.
			const QRect pill(4, row.top() + 1, viewport()->width() - 8, row.height() - 2);
			painter.setBrush(fill);
			painter.drawRoundedRect(pill, 6, 6);
		};
		if (currentIndex().isValid())
			paintRow(currentIndex(), QColor(169, 174, 182, 82));
		// The current row keeps the accent — hover only brightens the rest,
		// at the same white-0.08 the editor tabs use on hover.
		if (hoverIndex.isValid() && hoverIndex != currentIndex())
			paintRow(hoverIndex, QColor(255, 255, 255, 20));
	}
	QTreeWidget::paintEvent(event);
}

void QtFileSidebar::mouseMoveEvent(QMouseEvent *event)
{
	setHover(indexAt(event->pos()));
	QTreeWidget::mouseMoveEvent(event);
}

void QtFileSidebar::leaveEvent(QEvent *event)
{
	setHover(QModelIndex());
	QTreeWidget::leaveEvent(event);
}

void QtFileSidebar::setHover(const QModelIndex &index)
{
	if (index == hoverIndex)
		return;
	hoverIndex = index;
	viewport()->update();
}

void QtFileSidebar::updateIconSize()
{
	const int px = std::max(10, QFontMetrics(font()).height() - 8);
	setIconSize(QSize(px, px));
}

void QtFileSidebar::refreshIconScale()
{
	updateIconSize();
	// iconSize alone doesn't resize already-set icons (each QIcon keeps its
	// own raster) — re-resolve every row's icon at the new pixel size.
	std::function<void(QTreeWidgetItem *)> rescale = [&](QTreeWidgetItem *item) {
		FileNode *node = nodeOf(item);
		if (node)
			applyRowIcon(item, *node);
		for (int i = 0; i < item->childCount(); ++i)
			rescale(item->child(i));
	};
	for (int i = 0; i < topLevelItemCount(); ++i)
		rescale(topLevelItem(i));
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
	updateIconSize(); // icon size must be set before rows are created
	populate(invisibleRootItem(), tree.rootNode);
}

void QtFileSidebar::applyRowIcon(QTreeWidgetItem *item, const FileNode &node)
{
	// File-type SVGs carry intrinsic padding (smaller glyph in the same
	// viewBox) — rasterize them a notch larger than folders.
	const int px = std::max(10, iconSize().width());
	item->setIcon(0,
				  node.isDirectory
					  ? QtIconSet::byKey(node.isOpen ? "folder-open" : "folder", px)
					  : QtIconSet::forFile(QString::fromStdString(node.name),
										   std::max(12, px * 3 / 2)));
}

void QtFileSidebar::populate(QTreeWidgetItem *parentItem, FileNode &node)
{
	for (FileNode &child : node.children)
	{
		auto *item = new QTreeWidgetItem({QString::fromStdString(child.name)});
		item->setData(0, Qt::UserRole, QVariant::fromValue<void *>(&child));
		applyRowIcon(item, child);
		parentItem->addChild(item);
		if (child.isDirectory)
		{
			if (!child.children.empty())
				populate(item, child);
			item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
		}
	}
}
