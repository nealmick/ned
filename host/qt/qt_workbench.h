/*
	File: host/qt/qt_workbench.h
	Description: Workspace surface for the Qt host — counterpart of the
	ImGui host's Workbench (host/imgui/workbench.*). Documents live in tab
	groups (EditorGroup); groups sit in a recursive QSplitter tree the user
	reshapes by splitting or dragging tabs onto drop zones — the ImGui
	dockspace's drag-to-split, as native Qt. The welcome page replaces the
	tree while no documents are open. No floating editors, no layout
	persistence — ImGui parity on both.
*/

#pragma once

#include <QSplitter>
#include <QTabWidget>
#include <QWidget>

#include <QList>

class QDragEnterEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;
class QStackedLayout;
class QVBoxLayout;
class QtEditorView;

// Splitter handle that paints its own hairline. Stylesheet backgrounds
// never render on splitter handles in this app's translucent macOS window
// (the reason 1px handles showed as nothing at all), so the line is drawn
// here with the same tone the sidebar divider uses; accent tint on hover.
class NedSplitterHandle : public QSplitterHandle
{
	Q_OBJECT

  public:
	using QSplitterHandle::QSplitterHandle;

  protected:
	void paintEvent(QPaintEvent *event) override;
	void enterEvent(QEnterEvent *event) override;
	void leaveEvent(QEvent *event) override;
};

class NedSplitter : public QSplitter
{
	Q_OBJECT

  public:
	using QSplitter::QSplitter;

  protected:
	QSplitterHandle *createHandle() override;
};

class EditorGroup : public QTabWidget
{
	Q_OBJECT

  public:
	explicit EditorGroup(QWidget *parent = nullptr);

  Q_SIGNALS:
	// DnD relay — QtWorkbench owns the zone logic and performs the move.
	void tabDragged(EditorGroup *group, const QPoint &localPos);
	void tabDragLeft(EditorGroup *group);
	void tabDropped(EditorGroup *group, const QPoint &localPos);

  protected:
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dropEvent(QDropEvent *event) override;
};

class QtWorkbench : public QWidget
{
	Q_OBJECT

  public:
	explicit QtWorkbench(QWidget *parent = nullptr);

	// Shown whenever no documents are open. Takes ownership.
	void setWelcomePage(QWidget *page);

	// A workspace (folder) is open: the editor area is THE surface — the
	// welcome page hides for good (it only exists for the pre-folder
	// state), even when the last document closes.
	void setWorkspaceActive(bool active);

	// Adds an already-configured editor as a new tab in the ACTIVE group
	// (ImGui parity: new documents open into the focused split).
	void addEditor(QtEditorView *view, const QString &title, bool focus);

	// Tab bookkeeping used by the host shell.
	QList<QtEditorView *> views() const;
	int editorCount() const;
	EditorGroup *groupForView(const QtEditorView *view) const;
	QtEditorView *viewForPath(const QString &path) const;
	// Editor in the focused group's current tab (null when none).
	QtEditorView *activeView() const;
	// Re-create the per-group tab ✕ buttons at the current chrome scale
	// (font zoom changes their size/glyph; they otherwise refresh only on
	// the next tab switch).
	void refreshTabChrome();
	void setTabText(QtEditorView *view, const QString &text);
	void activateTabIndex(int index); // Ctrl+1..9 within the active group
	void closeActiveTab();
	void splitActive(Qt::Orientation orientation);

  Q_SIGNALS:
	// Emitted after a view was pulled out of its tab; the workbench deletes
	// the view itself (deleteLater) right after.
	void editorClosed(QtEditorView *view);

  protected:
	bool eventFilter(QObject *watched, QEvent *event) override;
	void paintEvent(QPaintEvent *event) override;

  private:
	enum class DropZone { Center, Left, Right, Top, Bottom };
	struct DropHint
	{
		DropZone zone = DropZone::Center;
		QRect preview;		  // highlight rect in QtWorkbench coordinates
		int insertIndex = -1; // tab position when landing on a tab bar
	};

	QList<EditorGroup *> groupsInOrder() const;
	QWidget *treeRoot() const;
	EditorGroup *makeGroup();
	QSplitter *makeSplitter(Qt::Orientation o);
	void relayout();
	void relayoutNode(QWidget *node);
	EditorGroup *splitGroup(EditorGroup *target, Qt::Orientation o, bool before);
	void dissolveGroup(EditorGroup *group); // drop an empty group, unwrap splitters
	void closeTab(EditorGroup *group, int index);
	void moveView(QtEditorView *view, EditorGroup *to, int index); // -1 = append
	void setActiveGroup(EditorGroup *group);
	void showTree(bool show);
	EditorGroup *groupForBar(const QTabBar *bar) const;

	DropHint dropHintFor(EditorGroup *group, const QPoint &localPos) const;
	void updateDropPreview(const DropHint &hint);
	void hideDropPreview();
	void startTabDrag(EditorGroup *group, int index, const QPoint &hotspot);
	void handleDrop(EditorGroup *target, const QPoint &localPos);
	void showTabContextMenu(EditorGroup *group, const QPoint &barPos);

	QStackedLayout *stack = nullptr;
	QWidget *welcomePage = nullptr;
	QVBoxLayout *welcomeLayout = nullptr;
	// ImGui parity (files.h showWelcomeScreen): welcome shows only until the
	// user opens a workspace OR a document — closing the last tab then lands
	// on the empty "No file open" editor tree, never back on welcome.
	bool welcomeDismissed = false;
	QWidget *treeHost = nullptr;
	QVBoxLayout *treeLayout = nullptr;
	EditorGroup *active = nullptr;
	QWidget *dropPreview = nullptr;
	EditorGroup *dropHintGroup = nullptr; // group the preview belongs to

	// Live state of a press on a group's tab bar (custom cross-group drag).
	EditorGroup *pressGroup = nullptr;
	int pressIndex = -1;
	QPoint pressPos;
	bool pressValid = false;
	// Set while a QDrag we started is running (drop handlers read it).
	EditorGroup *dragGroup = nullptr;
	int dragIndex = -1;
};
