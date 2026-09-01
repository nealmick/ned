/*
	File: views/qt/qt_editor_view.h
	Description: Qt editor surface — paints the document (text, syntax
	colors, selection, caret, gutter) from the backend-neutral core and
	feeds keyboard edits into EditorCommands. ImGui-free sibling of
	editor_frame + text/gutter/caret views.
*/

#pragma once

#include <QWidget>
#include <QIcon>
#include <QPixmap>

class Settings;

#include "../../editor_commands.h"
#include "../../editor_events.h"
#include "../../editor_operations.h"
#include "../../editor_state.h"
#include "../../editor_view_state.h"
#include "../../services/git/git_service.h"
#include "../../services/highlight/highlight_service.h"
#include "../../services/save_service.h"
#include "../../../util/project_undo.h"

class QScrollBar;
class QTimer;
class QtFindBar;

class QtEditorView : public QWidget
{
	Q_OBJECT

  public:
	explicit QtEditorView(Settings &appSettings, QWidget *parent = nullptr);
	~QtEditorView() override;

	// Git gutter + status (shared service).
	void openWorkspaceRoot(const std::string &root);

	// Host-facing queries (tab titles, dedup by path).
	EditorState &document() { return state; }
	EditorViewState &viewport() { return viewState; }
	EditorCommands &commandHandler() { return commands; }

	// Repaint after programmatic edits (find/replace).
	void repaintAndFollow();

	// Find bar (Cmd/Ctrl+F).
	void toggleFindBar();

	// Go-to-line (Cmd/Ctrl+;).
	void goToLineDialog();
	QString filePath() const { return QString::fromStdString(state.path); }
	bool isDirty() const { return state.dirty; }

	// Load a file into the document (empty path = untitled buffer).
	void openFile(const QString &path);

	// Emitted after edits (host refreshes tab title dirty marker).
	Q_SIGNALS:
	void documentEdited();

  protected:
	void paintEvent(QPaintEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void inputMethodEvent(QInputMethodEvent *event) override;
	QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
	void wheelEvent(QWheelEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	QSize sizeHint() const override;

  private:
	void scheduleBlink();
	int visibleLines() const;
	int maxScrollLine() const;
	// Widget position -> (row, column). x snap uses the monospace advance.
	int rowAtY(int y) const;
	int columnAtX(int row, int x) const;
	void setFontFromSettings();
	void afterEdit();

	Settings &appSettings;

	// Document core — same composition as the ImGui Editor / test fixture.
	std::string projectRoot = ".";
	ProjectUndo projectUndo;
	EditorState state;
	EditorEvents events;
	EditorOperations ops;
	EditorViewState viewState;
	EditorSave save;
	EditorHighlight highlight;
	EditorGit git;
	EditorCommands commands;

	// Presentation
	QScrollBar *scrollBar = nullptr;
	QTimer *blinkTimer = nullptr;
	QTimer *serviceTimer = nullptr;
	QtFindBar *findBar = nullptr;
	uint64_t lastVisualGen = 0;
	bool caretVisible = true;
	int lineHeightPx = 1;
	int charWidthPx = 1;
	int gutterWidthPx = 0;
	int titleBarPx = 26;
	QIcon fileIcon;
	int rainbowPhase = 0;
	bool dragging = false;
};
