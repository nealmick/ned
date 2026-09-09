#include "workbench.h"

#include "editor/views/qt/editor_frame.h"
#include "tab_close.h"

#include <QApplication>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QSplitter>
#include <QStackedLayout>
#include <QTabBar>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
const char kTabMime[] = "application/x-ned-editor-tab";
// Edge bands (fraction of the group's size) that trigger a split drop.
const qreal kEdgeBand = 0.25;
// Per-pane share of its splitter, as a dynamic property. QSplitter has no
// "distribute proportionally" mode — it stores raw pixels and its insert/
// remove defaults are unusable (size-hint slivers, freed space dumped on
// the last section) — so the tree owns explicit fractions instead and one
// relayout() pass applies them top-down after any structural change.
const char kFractionProp[] = "nedSplitFraction";

bool isTabDrag(const QMimeData *mime)
{
	return mime && mime->hasFormat(QLatin1String(kTabMime));
}

void setFraction(QWidget *widget, double fraction)
{
	widget->setProperty(kFractionProp, fraction);
}

double fractionOf(const QWidget *widget)
{
	const QVariant value = widget->property(kFractionProp);
	return value.isValid() ? value.toDouble() : 1.0;
}

// Recursive in-order walk of the splitter tree.
void collectGroups(QWidget *node, QList<EditorGroup *> &out)
{
	if (auto *group = qobject_cast<EditorGroup *>(node))
		out.append(group);
	else if (auto *splitter = qobject_cast<QSplitter *>(node))
		for (QWidget *child :
			 splitter->findChildren<QWidget *>(Qt::FindDirectChildrenOnly))
			if (qobject_cast<EditorGroup *>(child) || qobject_cast<QSplitter *>(child))
				collectGroups(child, out);
}
} // namespace

EditorGroup::EditorGroup(QWidget *parent) : QTabWidget(parent)
{
	setAcceptDrops(true); // tab drops land anywhere over the group
	// Keep splits usable: an empty group's size hint is just the tab bar,
	// which lets QSplitter squeeze it to a sliver.
	setMinimumSize(120, 80);
	// No documentMode: the app stylesheet owns tab geometry (uniform pills).
	// Close ✕ is a per-active-tab tool button (installed by Workbench).
	setTabsClosable(false);
	// Cross-group drag is custom (Workbench watches the bar), so the
	// built-in movable-drag stays off — our QDrag covers reorder too.
	setMovable(false);
	// Overflow = squeeze, never scroll arrows: pills shrink and filenames
	// elide (Safari-style). Fusion elides nothing by default (ElideNone),
	// which is what made the bar overflow into scrollers in the first
	// place — every tab stays visible and reachable this way.
	setUsesScrollButtons(false);
	tabBar()->setElideMode(Qt::ElideRight);
}

void EditorGroup::dragEnterEvent(QDragEnterEvent *event)
{
	if (isTabDrag(event->mimeData()))
	{
		event->acceptProposedAction();
		Q_EMIT tabDragged(this, event->position().toPoint());
	}
}

void EditorGroup::dragMoveEvent(QDragMoveEvent *event)
{
	if (isTabDrag(event->mimeData()))
	{
		event->acceptProposedAction();
		Q_EMIT tabDragged(this, event->position().toPoint());
	}
}

void EditorGroup::dragLeaveEvent(QDragLeaveEvent *event)
{
	Q_UNUSED(event);
	Q_EMIT tabDragLeft(this);
}

void EditorGroup::dropEvent(QDropEvent *event)
{
	if (isTabDrag(event->mimeData()))
	{
		event->acceptProposedAction();
		Q_EMIT tabDropped(this, event->position().toPoint());
	}
}

Workbench::Workbench(QWidget *parent) : QWidget(parent)
{
	stack = new QStackedLayout(this);
	stack->setContentsMargins(0, 0, 0, 0);

	// Page 0: welcome (installed by the host via setWelcomePage).
	auto *welcomeHost = new QWidget(this);
	welcomeLayout = new QVBoxLayout(welcomeHost);
	welcomeLayout->setContentsMargins(0, 0, 0, 0);
	stack->addWidget(welcomeHost);

	// Page 1: the splitter tree of editor groups.
	treeHost = new QWidget(this);
	treeLayout = new QVBoxLayout(treeHost);
	treeLayout->setContentsMargins(0, 0, 0, 0);
	treeLayout->setSpacing(0);
	stack->addWidget(treeHost);

	// Drop-zone highlight (the ImGui dock-preview look). Transparent for
	// mouse events so it never steals the drop it is previewing.
	dropPreview = new QWidget(this);
	dropPreview->setAttribute(Qt::WA_TransparentForMouseEvents, true);
	dropPreview->setAttribute(Qt::WA_StyledBackground, true);
	dropPreview->setStyleSheet(
		"background: rgba(169,174,182,0.22); "
		"border: 1px solid rgba(169,174,182,0.65); border-radius: 6px;");
	dropPreview->hide();

	treeLayout->addWidget(makeGroup());
}

void Workbench::setWelcomePage(QWidget *page)
{
	if (welcomePage)
		welcomePage->deleteLater();
	welcomePage = page;
	if (!page)
		return;
	page->setParent(this);
	while (welcomeLayout->count())
		delete welcomeLayout->takeAt(0); // stale spacer items only
	welcomeLayout->addWidget(page);
	if (editorCount() == 0)
		showTree(false);
}

void Workbench::setWorkspaceActive(bool active)
{
	if (active)
		welcomeDismissed = true;
	// Folder opened: welcome is done — the editor tree is the surface
	// from here on (empty tree included). Pre-folder: welcome shows.
	showTree(welcomeDismissed || editorCount() > 0);
}

void Workbench::addEditor(EditorFrame *view, const QString &title, bool focus)
{
	if (!active)
		active = effectiveGroup();
	if (!active)
	{
		active = makeGroup();
		treeLayout->addWidget(active);
	}
	const int index = active->addTab(view, title);
	view->installEventFilter(this); // FocusIn → setActiveGroup
	if (focus)
	{
		active->setCurrentIndex(index);
		setActiveGroup(active);
		view->setFocus(Qt::OtherFocusReason);
	}
	// Any document dismisses the welcome for good — a bare file opened
	// without a workspace (finder/recent) must not fall back to it later.
	welcomeDismissed = true;
	showTree(true);
}

QList<EditorFrame *> Workbench::views() const
{
	QList<EditorFrame *> out;
	for (EditorGroup *group : groupsInOrder())
		for (int i = 0; i < group->count(); ++i)
			if (auto *view = qobject_cast<EditorFrame *>(group->widget(i)))
				out.append(view);
	return out;
}

int Workbench::editorCount() const
{
	int total = 0;
	for (EditorGroup *group : groupsInOrder())
		total += group->count();
	return total;
}

EditorGroup *Workbench::groupForView(const EditorFrame *view) const
{
	for (EditorGroup *group : groupsInOrder())
		if (group->indexOf(const_cast<EditorFrame *>(view)) >= 0)
			return group;
	return nullptr;
}

EditorFrame *Workbench::viewForPath(const QString &path) const
{
	if (path.isEmpty())
		return nullptr; // untitled docs never dedup
	for (EditorFrame *view : views())
		if (view->filePath() == path)
			return view;
	return nullptr;
}

EditorFrame *Workbench::activeView() const
{
	return active ? qobject_cast<EditorFrame *>(active->currentWidget()) : nullptr;
}

void Workbench::setTabText(EditorFrame *view, const QString &text)
{
	if (EditorGroup *group = groupForView(view))
		group->setTabText(group->indexOf(view), text);
}

void Workbench::activateTab(int index)
{
	EditorGroup *group = effectiveGroup();
	if (!group)
		return;
	if (index < group->count())
	{
		group->setCurrentIndex(index);
		if (QWidget *page = group->widget(index))
			page->setFocus();
	}
}

void Workbench::closeActiveTab()
{
	if (active && active->count() > 0)
		closeTab(active, active->currentIndex());
}

void Workbench::splitActive(Qt::Orientation orientation)
{
	EditorGroup *target = effectiveGroup();
	if (!target)
		return;
	if (target->count() == 0)
		return; // splitting an empty group is a no-op
	EditorGroup *fresh = splitGroup(target, orientation, /*before=*/false);
	setActiveGroup(fresh); // new documents land in the fresh group
}

// --- splitter tree surgery -------------------------------------------------

QList<EditorGroup *> Workbench::groupsInOrder() const
{
	QList<EditorGroup *> out;
	if (QWidget *root = treeRoot())
		collectGroups(root, out);
	return out;
}

// Active group, falling back to the first group in tree order when none is
// marked active yet (shared by addEditor / activateTab / splitActive).
EditorGroup *Workbench::effectiveGroup() const
{
	if (active)
		return active;
	const QList<EditorGroup *> groups = groupsInOrder();
	return groups.isEmpty() ? nullptr : groups.first();
}

QWidget *Workbench::treeRoot() const
{
	return treeLayout->count() == 0 ? nullptr : treeLayout->itemAt(0)->widget();
}

void Workbench::refreshTabChrome()
{
	// Same path as the group's currentChanged handler: the active tab gets
	// a fresh ✕ sized for the new app font, the rest are cleared.
	for (EditorGroup *group : groupsInOrder())
		updateTabCloseButtons(group->tabBar(), group->currentIndex(), [group](int index) {
			Q_EMIT group->tabCloseRequested(index);
		});
}

EditorGroup *Workbench::makeGroup()
{
	auto *group = new EditorGroup(this);

	// Per-active-tab ✕ (same behavior the shell had on its single tab
	// widget; each group owns its own bar now). Shared button factory with
	// the terminal panel (tab_close.h).
	connect(group, &QTabWidget::currentChanged, this, [group](int current) {
		updateTabCloseButtons(group->tabBar(), current, [group](int index) {
			Q_EMIT group->tabCloseRequested(index);
		});
	});
	connect(group, &QTabWidget::tabCloseRequested, this, [this, group](int index) {
		closeTab(group, index);
	});

	// Cross-group tab drag: watch presses on the bar, relay drop events.
	group->tabBar()->installEventFilter(this);
	group->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(group->tabBar(),
			&QTabBar::customContextMenuRequested,
			this,
			[this, group](const QPoint &pos) { showTabContextMenu(group, pos); });
	connect(
		group, &EditorGroup::tabDragged, this, [this](EditorGroup *g, const QPoint &p) {
			dropHintGroup = g;
			updateDropPreview(dropHintFor(g, p));
		});
	connect(group, &EditorGroup::tabDragLeft, this, [this](EditorGroup *g) {
		if (dropPreview->isVisible() && dropHintGroup == g)
			hideDropPreview();
	});
	connect(
		group, &EditorGroup::tabDropped, this, [this](EditorGroup *g, const QPoint &p) {
			handleDrop(g, p);
		});
	return group;
}

// --- painted 1px split handles ---------------------------------------------

void NedSplitterHandle::paintEvent(QPaintEvent *)
{
	// Same tone as the sidebar's divider line (white, alpha 23); the
	// accent-grey tint while hovered makes the 1px grab line findable.
	QPainter painter(this);
	painter.fillRect(
		rect(), underMouse() ? QColor(169, 174, 182, 115) : QColor(255, 255, 255, 23));
}

void NedSplitterHandle::enterEvent(QEnterEvent *event)
{
	QSplitterHandle::enterEvent(event);
	update();
}

void NedSplitterHandle::leaveEvent(QEvent *event)
{
	QSplitterHandle::leaveEvent(event);
	update();
}

QSplitterHandle *NedSplitter::createHandle()
{
	return new NedSplitterHandle(orientation(), this);
}

QSplitter *Workbench::makeSplitter(Qt::Orientation o)
{
	auto *splitter = new NedSplitter(o, this);
	splitter->setChildrenCollapsible(false);
	splitter->setHandleWidth(1);
	// A handle drag re-grounds the stored fractions in reality, so the
	// next structural change preserves manual ratios instead of snapping
	// back. Also fires for our own setSizes — harmless, it's idempotent.
	connect(splitter, &QSplitter::splitterMoved, this, [splitter] {
		const QList<int> sizes = splitter->sizes();
		int sum = 0;
		for (int v : sizes)
			sum += v;
		if (sum <= 0)
			return;
		for (int i = 0; i < splitter->count() && i < sizes.size(); ++i)
			setFraction(splitter->widget(i), sizes.at(i) / double(sum));
	});
	return splitter;
}

// One top-down pass applying every pane's stored fraction. Runs after any
// structural change (split, insert, dissolve, unwrap) — the single place
// that touches splitter sizes, so insert/remove edge cases can't disagree.
void Workbench::relayoutNode(QWidget *node)
{
	auto *splitter = qobject_cast<QSplitter *>(node);
	if (!splitter)
		return;
	QList<int> weights;
	for (int i = 0; i < splitter->count(); ++i)
	{
		splitter->setStretchFactor(i, 1); // resize growth follows fractions
		weights << qRound(fractionOf(splitter->widget(i)) * 1000.0);
	}
	splitter->setSizes(weights);
	for (QWidget *child : splitter->findChildren<QWidget *>(Qt::FindDirectChildrenOnly))
		if (qobject_cast<EditorGroup *>(child) || qobject_cast<QSplitter *>(child))
			relayoutNode(child);
}

void Workbench::relayout()
{
	if (QWidget *root = treeRoot())
		relayoutNode(root);
}

EditorGroup *Workbench::splitGroup(EditorGroup *target, Qt::Orientation o, bool before)
{
	EditorGroup *fresh = makeGroup();
	auto *parent = qobject_cast<QSplitter *>(target->parentWidget());

	if (!parent) // target is the tree root: wrap it in a new splitter
	{
		auto *splitter = makeSplitter(o);
		treeLayout->removeWidget(target);
		if (before)
		{
			splitter->addWidget(fresh);
			splitter->addWidget(target);
		} else
		{
			splitter->addWidget(target);
			splitter->addWidget(fresh);
		}
		setFraction(target, 0.5);
		setFraction(fresh, 0.5);
		treeLayout->addWidget(splitter);
		relayout();
		return fresh;
	}

	// Fractions are per-parent: the target's share halves, the newcomer
	// takes the other half — untouched siblings never move (VS Code /
	// ImGui dock-drop semantics).
	if (parent->orientation() == o) // same direction: plain sibling
	{
		const int index = parent->indexOf(target) + (before ? 0 : 1);
		const double share = fractionOf(target) / 2.0;
		parent->insertWidget(index, fresh);
		setFraction(target, share);
		setFraction(fresh, share);
	} else // different direction: nest a child splitter around target
	{
		auto *child = makeSplitter(o);
		const int index = parent->indexOf(target);
		parent->insertWidget(index, child);
		setFraction(child, fractionOf(target)); // child inherits the slot
		if (before)
		{
			child->addWidget(fresh);
			child->addWidget(target);
		} else
		{
			child->addWidget(target);
			child->addWidget(fresh);
		}
		setFraction(target, 0.5); // relative to the new child
		setFraction(fresh, 0.5);
	}
	relayout();
	return fresh;
}

void Workbench::dissolveGroup(EditorGroup *group)
{
	QWidget *parent = group->parentWidget();
	group->setParent(nullptr); // detaches from the splitter
	group->deleteLater();
	if (parent == treeHost)
		return; // root removed — tree is empty, welcome takes the page

	auto *splitter = qobject_cast<QSplitter *>(parent);
	if (!splitter)
		return;
	if (splitter->count() > 1)
	{
		// Survivors keep their fractions; the freed share spreads by
		// stretch. relayout applies it — no pixel bookkeeping.
		relayout();
		return;
	}
	if (splitter->count() == 0)
	{
		// Structural oddity (empty splitter): drop it entirely.
		splitter->setParent(nullptr);
		splitter->deleteLater();
		relayout();
		return;
	}
	// Unwrap: a splitter holding a single child is replaced by that child,
	// which inherits the splitter's fraction in the grandparent.
	QWidget *only = splitter->widget(0);
	QWidget *grand = splitter->parentWidget();
	if (grand == treeHost)
	{
		treeLayout->removeWidget(splitter);
		treeLayout->addWidget(only);
	} else if (auto *grandSplitter = qobject_cast<QSplitter *>(grand))
	{
		grandSplitter->insertWidget(grandSplitter->indexOf(splitter), only);
		setFraction(only, fractionOf(splitter));
	} else
		return;
	splitter->setParent(nullptr);
	splitter->deleteLater();
	relayout();
}

void Workbench::closeTab(EditorGroup *group, int index)
{
	QWidget *page = group->widget(index);
	group->removeTab(index);
	if (auto *view = qobject_cast<EditorFrame *>(page))
	{
		Q_EMIT editorClosed(view);
		view->deleteLater();
	}
	if (group->count() == 0 && group != treeRoot())
	{
		if (active == group)
			active = nullptr;
		dissolveGroup(group);
	}
	if (editorCount() == 0)
		showTree(welcomeDismissed); // last doc closed: "No file open" tree
	else if (!active || active->count() == 0)
	{
		const QList<EditorGroup *> groups = groupsInOrder();
		if (!groups.isEmpty())
			setActiveGroup(groups.first());
	}
}

void Workbench::moveView(EditorFrame *view, EditorGroup *to, int index)
{
	EditorGroup *from = groupForView(view);
	if (!from || !to)
		return;
	const int current = from->indexOf(view);
	if (current < 0)
		return;
	const QString text = from->tabText(current);

	if (from == to && index >= 0)
	{
		if (index == current || index == current + 1)
			return; // no-op position
		from->removeTab(current);
		int at = index > current ? index - 1 : index;
		if (at > from->count())
			at = from->count();
		from->insertTab(at, view, text);
		from->setCurrentIndex(at);
		return;
	}

	from->removeTab(current);
	const int at = (index < 0 || index > to->count()) ? to->count() : index;
	to->insertTab(at, view, text);
	to->setCurrentIndex(at);
	setActiveGroup(to);
	if (from->count() == 0 && from != to && from != treeRoot())
	{
		if (active == from)
			active = nullptr;
		dissolveGroup(from);
	}
}

void Workbench::setActiveGroup(EditorGroup *group)
{
	if (!group)
	{
		active = nullptr;
		return;
	}
	if (active == group)
		return;
	active = group;
	// VS Code behavior: an empty group that lost focus dissolves away —
	// a split that was never used never lingers.
	for (EditorGroup *g : groupsInOrder())
		if (g != active && g->count() == 0 && g != treeRoot())
			dissolveGroup(g);
}

void Workbench::showTree(bool show)
{
	stack->setCurrentWidget(show ? treeHost : stack->widget(0));
}

void Workbench::paintEvent(QPaintEvent *event)
{
	QWidget::paintEvent(event);
	// Empty-workspace state: the tree page (transparent) is showing with
	// no documents — a quiet centered hint shows through it, under the
	// empty group's tab strip.
	if (stack->currentWidget() != treeHost || editorCount() > 0)
		return;
#ifdef __APPLE__
	const QString hint = QStringLiteral("⌘O open folder   ·   ⌘P find file");
#else
	const QString hint = QStringLiteral("Ctrl+O open folder   ·   Ctrl+P find file");
#endif
	QPainter p(this);
	const QRect area = rect().adjusted(0, 72, 0, -56);
	QColor ink = palette().windowText().color();
	QFont f = p.font();
	f.setPointSizeF(f.pointSizeF() * 1.5);
	ink.setAlpha(140);
	p.setFont(f);
	p.setPen(ink);
	p.drawText(area, Qt::AlignHCenter | Qt::AlignTop, QStringLiteral("No file open"));
	f.setPointSizeF(f.pointSizeF() / 1.5 * 0.92);
	ink.setAlpha(85);
	p.setFont(f);
	p.setPen(ink);
	p.drawText(area.adjusted(0, 52, 0, 52), Qt::AlignHCenter | Qt::AlignTop, hint);
}

EditorGroup *Workbench::groupForBar(const QTabBar *bar) const
{
	for (EditorGroup *group : groupsInOrder())
		if (group->tabBar() == bar)
			return group;
	return nullptr;
}

// --- tab dragging ----------------------------------------------------------

Workbench::DropHint Workbench::dropHintFor(EditorGroup *group,
										   const QPoint &localPos) const
{
	DropHint hint;
	const QRect barRect = group->tabBar()->frameGeometry();
	// Hovering a tab bar always means "land as a tab" (VS Code), never a
	// split — even on the source bar (reorder).
	if (barRect.contains(localPos))
	{
		hint.zone = DropZone::Center;
		hint.insertIndex = group->tabBar()->tabAt(localPos);
		if (hint.insertIndex >= 0)
			hint.preview =
				QRect(group->tabBar()
						  ->tabRect(hint.insertIndex)
						  .translated(group->tabBar()->mapTo(this, QPoint(0, 0))));
		else
			hint.preview = QRect(group->mapTo(this, QPoint(0, 0)), group->size())
							   .adjusted(4, 4, -4, -4);
		return hint;
	}

	const qreal w = group->width(), h = group->height();
	if (w <= 0 || h <= 0)
	{
		hint.preview = QRect(group->mapTo(this, QPoint(0, 0)), group->size());
		return hint;
	}
	const QRect full = QRect(group->mapTo(this, QPoint(0, 0)), group->size());
	const qreal rx = localPos.x() / w, ry = localPos.y() / h;
	if (rx < kEdgeBand)
	{
		hint.zone = DropZone::Left;
		hint.preview = full.adjusted(0, 0, -full.width() / 2, 0);
	} else if (rx > 1.0 - kEdgeBand)
	{
		hint.zone = DropZone::Right;
		hint.preview = full.adjusted(full.width() / 2, 0, 0, 0);
	} else if (ry < kEdgeBand)
	{
		hint.zone = DropZone::Top;
		hint.preview = full.adjusted(0, 0, 0, -full.height() / 2);
	} else if (ry > 1.0 - kEdgeBand)
	{
		hint.zone = DropZone::Bottom;
		hint.preview = full.adjusted(0, full.height() / 2, 0, 0);
	} else
	{
		hint.zone = DropZone::Center;
		hint.preview = full.adjusted(4, 4, -4, -4);
	}
	return hint;
}

void Workbench::updateDropPreview(const DropHint &hint)
{
	// The hint's group is the drag target (set by the caller's context);
	// geometry is already mapped into Workbench coordinates.
	if (dropPreview->geometry() != hint.preview)
		dropPreview->setGeometry(hint.preview);
	dropPreview->raise();
	dropPreview->show();
}

void Workbench::hideDropPreview()
{
	dropPreview->hide();
	dropHintGroup = nullptr;
}

void Workbench::startTabDrag(EditorGroup *group, int index, const QPoint &hotspot)
{
	auto *bar = group->tabBar();
	// Heap allocations on purpose: QDrag owns (deletes) its mime data, and
	// exec() runs a nested event loop where deferred deletes fire — if the
	// source group dissolves on drop, a stack QDrag parented to `bar` (or
	// a stack QMimeData) would be destroyed twice.
	auto *drag = new QDrag(this); // outlives any dissolved group
	auto *mime = new QMimeData;
	QByteArray payload;
	const quintptr source = reinterpret_cast<quintptr>(group);
	payload.resize(sizeof(source) + sizeof(index));
	memcpy(payload.data(), &source, sizeof(source));
	memcpy(payload.data() + sizeof(source), &index, sizeof(index));
	mime->setData(QLatin1String(kTabMime), payload);
	drag->setMimeData(mime);
	// QTabBar::tabPixmap() is protected — grab the rendered tab rect.
	const QPixmap pix = bar->grab(bar->tabRect(index));
	if (!pix.isNull())
	{
		drag->setPixmap(pix);
		drag->setHotSpot(hotspot);
	}
	dragGroup = group;
	dragIndex = index;
	drag->exec(Qt::MoveAction);
	// Drop (if any) was handled inside exec by the target's dropEvent.
	drag->deleteLater();
	dragGroup = nullptr;
	dragIndex = -1;
	pressGroup = nullptr; // may have been dissolved during the drop
	pressValid = false;
	hideDropPreview();
}

void Workbench::handleDrop(EditorGroup *target, const QPoint &localPos)
{
	EditorGroup *source = dragGroup;
	if (!source || dragIndex < 0 || dragIndex >= source->count())
		return;
	auto *view = qobject_cast<EditorFrame *>(source->widget(dragIndex));
	if (!view)
		return;

	const DropHint hint = dropHintFor(target, localPos);
	switch (hint.zone)
	{
	case DropZone::Left:
		moveView(view, splitGroup(target, Qt::Horizontal, /*before=*/true), 0);
		break;
	case DropZone::Right:
		moveView(view, splitGroup(target, Qt::Horizontal, /*before=*/false), 0);
		break;
	case DropZone::Top:
		moveView(view, splitGroup(target, Qt::Vertical, /*before=*/true), 0);
		break;
	case DropZone::Bottom:
		moveView(view, splitGroup(target, Qt::Vertical, /*before=*/false), 0);
		break;
	case DropZone::Center:
		if (target == source)
		{
			// Reorder inside the source bar at the hovered position.
			int at = hint.insertIndex;
			if (at < 0)
				at = -1; // past the last tab: append
			else if (at > dragIndex)
				++at; // compensate for the removal shift
			moveView(view, target, at);
		} else
			moveView(view, target, hint.insertIndex < 0 ? -1 : hint.insertIndex);
		break;
	}
	hideDropPreview();
}

void Workbench::showTabContextMenu(EditorGroup *group, const QPoint &barPos)
{
	const int index = group->tabBar()->tabAt(barPos);
	QMenu menu(group);
	QAction splitRight(QStringLiteral("Split Right"), &menu);
	QAction splitDown(QStringLiteral("Split Down"), &menu);
	QAction move(QStringLiteral("Move to Other Group"), &menu);
	QAction close(QStringLiteral("Close"), &menu);
	const QList<EditorGroup *> groups = groupsInOrder();
	move.setEnabled(index >= 0 && groups.size() > 1);
	close.setEnabled(index >= 0);
	connect(&splitRight, &QAction::triggered, this, [this, group] {
		active = group;
		splitActive(Qt::Horizontal);
	});
	connect(&splitDown, &QAction::triggered, this, [this, group] {
		active = group;
		splitActive(Qt::Vertical);
	});
	connect(&move, &QAction::triggered, this, [this, group, index] {
		const QList<EditorGroup *> all = groupsInOrder();
		if (all.size() < 2 || index < 0 || index >= group->count())
			return;
		const int me = all.indexOf(group);
		EditorGroup *other = all[(me + 1) % all.size()];
		if (auto *view = qobject_cast<EditorFrame *>(group->widget(index)))
			moveView(view, other, -1);
	});
	connect(&close, &QAction::triggered, this, [this, group, index] {
		if (index >= 0 && index < group->count())
			closeTab(group, index);
	});
	menu.addAction(&splitRight);
	menu.addAction(&splitDown);
	menu.addSeparator();
	menu.addAction(&move);
	menu.addAction(&close);
	menu.exec(group->tabBar()->mapToGlobal(barPos));
}

bool Workbench::eventFilter(QObject *watched, QEvent *event)
{
	if (event->type() == QEvent::FocusIn)
	{
		if (auto *view = qobject_cast<EditorFrame *>(watched))
			if (EditorGroup *group = groupForView(view))
				setActiveGroup(group);
	} else if (auto *bar = qobject_cast<QTabBar *>(watched))
	{
		const auto type = event->type();
		if (type == QEvent::MouseButtonPress)
		{
			auto *mouse = static_cast<QMouseEvent *>(event);
			if (mouse->button() == Qt::LeftButton)
			{
				const int index = bar->tabAt(mouse->position().toPoint());
				if (index >= 0)
				{
					pressGroup = groupForBar(bar);
					if (pressGroup)
					{
						pressIndex = index;
						pressPos = mouse->position().toPoint();
						pressValid = true;
						setActiveGroup(pressGroup);
					}
				}
			}
		} else if (type == QEvent::MouseMove && pressValid && pressGroup &&
				   watched == pressGroup->tabBar())
		{
			auto *mouse = static_cast<QMouseEvent *>(event);
			const QPoint pos = mouse->position().toPoint();
			if ((pos - pressPos).manhattanLength() >= QApplication::startDragDistance())
			{
				const QPoint hotspot =
					pressPos - pressGroup->tabBar()->tabRect(pressIndex).topLeft();
				pressValid = false;
				startTabDrag(pressGroup, pressIndex, hotspot);
				return true; // the drag owns the rest of this gesture
			}
		} else if (type == QEvent::MouseButtonRelease)
			pressValid = false;
	}
	return QWidget::eventFilter(watched, event);
}
