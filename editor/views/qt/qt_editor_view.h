/*
	File: views/qt/qt_editor_view.h
	Description: Qt editor surface — paints the document (text, syntax
	colors, selection, caret, gutter) from the backend-neutral core and
	feeds keyboard edits into EditorCommands. ImGui-free sibling of
	editor_frame + text/gutter/caret views.
*/

#pragma once

#include <QElapsedTimer>
#include <QGlyphRun>
#include <QIcon>
#include <QPixmap>
#include <QRawFont>
#include <QString>
#include <QWidget>
#include <map>
#include <vector>

class Settings;

#include "../../../util/project_undo.h"
#include "../../editor_commands.h"
#include "../../editor_events.h"
#include "../../editor_operations.h"
#include "../../editor_state.h"
#include "../../editor_view_state.h"
#include "../../services/git/git_service.h"
#include "../../services/highlight/highlight_service.h"
#include "../../services/save_service.h"
#include "../wrap_layout.h"

class QElapsedTimer;
class QLineEdit;
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

	// Reload font + metrics from the settings profile.
	void applyProfileFont()
	{
		setFontFromSettings();
		update();
	}

	// Host-facing queries (tab titles, dedup by path).
	EditorState &document() { return state; }
	EditorViewState &viewport() { return viewState; }
	EditorCommands &commandHandler() { return commands; }

	// Repaint after programmatic edits (find/replace).
	void repaintAndFollow();

	// Keep the caret inside the viewport (ImGui: revealCursor).
	void revealCaret();
	void showContextMenu(const QPoint &pos);

	// Minimap (right strip): renders one 2px row per document line with
	// syntax colors; click/drag scrolls. ImGui minimap_view parity.
	bool minimapEnabled() const;
	int minimapWidth() const;
	void paintMinimap(QPainter &painter);
	void minimapScrollTo(int y);

	bool wordWrapEnabled() const;
	int textAreaWidth() const;
	// Total scrollable lines (visual lines when wrapping).
	int totalLines() const;
	void refreshWrap();

	// --- Tab-expanded rendering model -------------------------------
	// Tabs expand to kTabSize (4) monospace cells. Per visible row we build
	// the expanded QString plus byte<->visual-column maps; text, caret,
	// selection and hit-testing all read the same maps (no drift).
	struct RowText
	{
		QString expanded;			   // tab-expanded text
		std::vector<int> byteToVisual; // index by byte offset
		std::vector<int> visualToByte; // index by visual column
	};
	RowText expandRow(int row) const;
	// Pixel x (from textLeft) of a byte column on a row.
	qreal xAtByteColumn(int row, int byteColumn, int segmentStart = 0) const;
	// Inverse for mouse hit-testing (segmentStart for wrapped rows).
	int byteColumnAtX(int row, qreal x, int segmentStart = 0) const;
	qreal charWidthF() const;

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
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	bool eventFilter(QObject *watched, QEvent *event) override;
	bool event(QEvent *event) override;
	QSize sizeHint() const override;

  private:
	void scheduleBlink();
	int visibleLines() const;
	int maxScrollLine() const;
	// Widget position -> (row, column). x snap uses the monospace advance.
	// y -> row + wrap-segment start (byte column of the segment).
	struct RowHit
	{
		int row = 0;
		int segmentStart = 0;
	};
	RowHit hitTestY(int y) const;
	int rowAtY(int y) const;
	int columnAtX(int row, int x, int segmentStart = 0) const;
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
	QTimer *serviceTimer = nullptr;
	QElapsedTimer blinkClock;
	WrapLayout wrap;
	QLineEdit *lineJumpInput = nullptr;
	QtFindBar *findBar = nullptr;
	uint64_t lastVisualGen = 0;
	bool caretVisible = true;
	int lineHeightPx = 1;
	qreal cellWidth = 9.0;
	int gutterWidthPx = 0;
	int titleBarPx = 26;
	QIcon fileIcon;
	bool dragging = false;
	bool minimapDragging = false;
	QString minimapCacheKey;
	QPixmap minimapCache;

	void paintTextRow(
		QPainter &painter, int row, int y, int fromByte, int toByte, qreal textLeft);
};