/*
	File: views/qt/editor_frame.h
	Description: Qt editor surface — the QWidget composition root over the
	backend-neutral core, parallel of views/imgui/editor_frame.h. The
	class is one widget, but its member definitions are split like the
	ImGui view classes: editor_frame.cpp (frame/scroll/file), editor_input
	.cpp (keyboard/mouse/hover), text_view.cpp, gutter_view.cpp,
	caret_view.cpp, title_bar_view.cpp, minimap_view.cpp.
*/

#pragma once

#include <QElapsedTimer>
#include <QIcon>
#include <QString>
#include <QWidget>
#include <cstdint>
#include <functional>
#include <vector>

class QPainter;
class Settings;

#include "../../../util/project_undo.h"
#include "../../editor_commands.h"
#include "../../editor_events.h"
#include "../../editor_operations.h"
#include "../../editor_state.h"
#include "../../editor_view_state.h"
#include "../../platform/lsp_editor.h"
#include "../../services/diagnostics/diagnostics_store.h"
#include "../../services/git/git_service.h"
#include "../../services/git/line_diff.h"
#include "../../services/highlight/highlight_service.h"
#include "../../services/save_service.h"
#include "../../util/hover_trigger.h"
#include "../wrap_layout.h"
#include "caret_view.h"
#include "editor_input.h"
#include "gutter_view.h"
#include "minimap_view.h"
#include "row_text.h"
#include "text_view.h"
#include "title_bar_view.h"

class QElapsedTimer;
class QScrollBar;
class QTimer;
class FindBar;
class HoverTooltip;

class EditorFrame : public QWidget, public LSPEditor
{
	Q_OBJECT

  public:
	explicit EditorFrame(Settings &appSettings, QWidget *parent = nullptr);
	~EditorFrame() override;

	// --- LSPEditor seam (lsp core queries/jumps through this; ImGui's
	// counterpart is EditorApi) ------------------------------------------
	void getCaret(int &row, int &column) const override;
	std::string line(int row) const override;
	const std::string &path() const override;
	const std::string &languageId() const override;
	void requestCursorCenter(int row, int column) override;
	NedColor defaultTextColor() const override;
	NedColor syntaxColor(ThemeSlot slot) const override;

	// --- LSP host wiring --------------------------------------------------
	// Diagnostics store (owned by LSPClient); null = no diagnostics.
	void setDiagnostics(const LSPDiagnostics *store);
	EditorEvents &editorEvents() { return events; }
	std::string documentText() const { return state.join(); }
	int documentVersion() const { return state.version; }

	// Hover (VSCode-style delay) arbitration: when the trigger fires, the
	// view shows diagnostic tooltips itself (gutter/squiggle zones) and
	// hands Text-zone cells to the observer (LSP symbol hover) only when
	// no diagnostic covers them. Info with active=false = dismissed.
	using HoverObserver = std::function<void(const HoverTrigger::Info &info)>;
	void setHoverObserver(HoverObserver observer) { hoverObserver = std::move(observer); }

	// Pixel position of the primary caret (widget coords) + line height —
	// LSP hover anchors its tooltip below the caret.
	QPoint caretWidgetPos() const { return caretView.caretWidgetPos(); }
	int caretLineHeight() const { return lineHeightPx; }

	// Git gutter + status (shared service).
	std::string gitChangesSummary() const { return titleBarView.gitSummary(); }

	void openWorkspaceRoot(const std::string &root);

	// Reload font + metrics from the settings profile.
	void applyProfileFont()
	{
		setFontFromSettings();
		// The zoom shortcut (Cmd +/-) lands here WITHOUT the host's
		// apply-font pass — re-clamp the viewport and re-sync scroll ranges
		// (line height changed, so visibleLines/maxScroll/wrap width all
		// moved; skipping this could leave scrollPx past the max and the
		// first wheel-ups dead-clamped to the bottom).
		refreshWrap();
		update();
	}

	// Host-facing queries (tab titles, dedup by path).
	EditorState &document() { return state; }
	EditorViewState &viewport() { return viewState; }
	EditorCommands &commandHandler() { return commands; }

	// Repaint after programmatic edits (find/replace).
	void repaintAndFollow();

	// Keep the caret inside the viewport (ImGui: EditorViewState::revealCaret).
	void revealCaret();
	void showContextMenu(const QPoint &pos);

	// Minimap (right strip): density map like the ImGui backend — per-line
	// colored runs (syntax spans dimmed), visible window only, continuous
	// slider, click/drag/wheel scroll. Vertical scrollbar hides while the
	// minimap is on. Strip geometry + density-run cache live in MinimapView.
	bool minimapEnabled() const;
	int minimapWidth() const;

	bool wordWrapEnabled() const;
	int textAreaWidth() const;
	// Total scrollable lines (visual lines when wrapping).
	int totalLines() const;
	void refreshWrap();

	// --- Horizontal scroll (wrap off; ImGui editor_view_scroll parity) ---
	// Longest-line cache is incremental like the ImGui frame's: edits mark a
	// row span; the max only rescans that span unless the old longest row
	// itself shrank (then it stays as a safe overestimate).
	qreal maxScrollPxX();
	void setScrollXPixels(qreal px);
	void refreshLongestLine(int lo, int hi); // full span scan
	void ensureLongestLine();				 // lazily rebuild when dirty
	// Thin overlay bar (ImGui HorizontalScrollbar parity): visible only when
	// wrap is off and lines overflow. Mirrors scrollPxX; dragging moves it.
	void syncHScrollBar();

	// Re-ensure the shared wrap layout against the current width before any
	// read (ImGui re-ensures every frame; Qt has no frame loop, so each
	// entry point that maps rows<->visual lines refreshes it).
	void ensureWrapFresh();

	// Monospace cell width — the single width source for the whole view.
	qreal charWidthF() const;

	// Find bar (Cmd/Ctrl+F). Shown, it sits BELOW the title strip and
	// displaces the text area (topInset) instead of overlaying it.
	void toggleFindBar();
	void closeFindBar();

	// Go-to-line (Cmd/Ctrl+;).
	void goToLineDialog();
	QString filePath() const { return QString::fromStdString(state.path); }
	bool isDirty() const { return state.dirty; }

	// Load a file into the document (empty path = untitled buffer).
	void openFile(const QString &path);

	// --- Diff view (git staged/unstaged) ----------------------------------
	// Loads a read-only GitHub-style unified diff of old vs new lines.
	// The buffer is untitled (state.path stays empty): no tab dedup against
	// the real file, no LSP didOpen, no ProjectUndo key. languageId comes
	// from displayPath so tree-sitter highlighting still runs.
	enum class DiffSide { Staged, Unstaged };
	void openDiff(const QString &displayPath,
				  DiffSide side,
				  const std::vector<std::string> &oldLines,
				  const std::vector<std::string> &newLines);
	bool isDiffView() const { return diffActive; }
	const QString &diffTargetPath() const { return diffTarget; }
	DiffSide diffTargetSide() const { return diffSideValue; }
	bool readOnly() const { return diffActive; }

	// Re-fetch the title icon at the current icon scale.
	void reloadFileIcon() { titleBarView.reloadIcon(); }

	// Theme changed: refresh tree-sitter's cached theme colors and
	// re-highlight (the ImGui host's EditorApi::forceColorUpdate).
	void forceColorUpdate();

	// Emitted after edits (host refreshes tab title dirty marker).
  Q_SIGNALS:
	void documentEdited();
	void fontZoomed();

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
	void leaveEvent(QEvent *event) override;
	bool event(QEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void focusOutEvent(QFocusEvent *event) override;
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
	// Top of the text area: below the title strip and, when open, below
	// the find bar that displaces the document.
	int topInset() const { return titleBarPx + findBarPx; }

	// --- Diagnostics (LSP) -------------------------------------------------

	void showDiagnosticTooltip(const std::vector<DiagnosticItem> &items,
							   const QPoint &globalPos);

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
	QScrollBar *hScrollBar = nullptr;
	QTimer *serviceTimer = nullptr;
	QElapsedTimer blinkClock;
	WrapLayout wrap;
	// Paint/input components — bodies live in the *_view.cpp files like the
	// ImGui backend's view classes (they reach back through friendship).
	TextView textView;
	GutterView gutterView;
	CaretView caretView;
	TitleBarView titleBarView;
	EditorInput input;

	friend class TextView;
	friend class GutterView;
	friend class CaretView;
	friend class TitleBarView;
	friend class EditorInput;
	friend class MinimapView;
	MinimapView minimapView; // paint + interact + run cache (minimap_view.h)
	FindBar *findBar = nullptr;
	// Height of the open find bar (0 when closed) — the text area starts
	// at topInset() = titleBarPx + findBarPx below it.
	int findBarPx = 0;
	uint64_t lastVisualGen = 0;
	std::string lastGitChanges;
	bool caretVisible = true;

	// Caret renders only for the focused editor: docked siblings sharing a
	// split must not each draw their own caret. Focus on a child (find bar)
	// still counts; no focus anywhere (headless render checks) keeps it on.
	bool caretActive() const;
	int lineHeightPx = 1;
	qreal cellWidth = 9.0;
	int gutterWidthPx = 0;
	int titleBarPx = 26;
	QIcon fileIcon;
	bool dragging = false;
	// Pixel scroll state (scrollPx/scrollPxX + wheel remainders) lives in
	// the shared EditorViewState; the syncing flags guard the QScrollBar
	// mirrors below against feedback loops.
	bool syncingScroll = false;
	bool syncingScrollX = false;
	// Longest-line cache backing the horizontal scroll range (stays here:
	// measured in the widget's monospace cell width, rebuilt on font zoom).
	qreal widthMaxPx = 0.0;
	int widthLongestRow = -1;
	bool widthDirty = true;

	// lineRef scratch (mutable: fetched from const paint/hit-test paths).
	mutable std::string scratchLine;
	mutable int scratchRow = -1;
	mutable uint64_t scratchGen = 0;

	// Diagnostics (LSP publish; painted as squiggles + gutter marks).
	const LSPDiagnostics *diagStore = nullptr;
	std::uint64_t diagRevisionSeen = 0; // repaint trigger in the service timer

	// Diagnostic hover card (shared styled tooltip).
	HoverTooltip *diagTip = nullptr;

	// Diff view: per-buffer-row classification parallel to the document
	// (row i of the buffer ↔ diffRows[i]); empty when not a diff view.
	std::vector<DiffOp> diffRows;
	bool diffActive = false;
	QString diffTarget;
	DiffSide diffSideValue = DiffSide::Unstaged;
	void paintDiffBackgrounds(QPainter &painter, int firstRow, int rows, qreal yBase);

	// Hover trigger + observer (see setHoverObserver).
	HoverTrigger hoverTrigger;
	HoverObserver hoverObserver;
	HoverTrigger::Info liveHoverInfo; // last dispatched state
	QPoint lastHoverPos;

	int maxScrollPx() const;
	void setScrollPixels(qreal px);
	int firstVisualLine() const;
	qreal rowYBase() const;

	// --- Tab-expanded rendering model (row_text.h) ------------------
	// Tabs expand to kTabSize (4) monospace cells. Text, caret, selection
	// and hit-testing all read the same cell maps (no drift);
	// segmentStart rebases tab stops at a wrap-segment edge.
	//
	// Only a WINDOW of a line is ever expanded — the viewport's columns
	// (wrap off) or one wrap segment — so a multi-megabyte single-line
	// file costs O(window) per paint/hit-test, never O(line).

	// The row's text through a one-entry cache — state.line() returns by
	// value and the paint/hit-test paths re-fetch the same row several
	// times per frame. Keep at most ONE reference live at a time.
	const std::string &lineRef(int row) const;
	// Byte window the coordinate helpers work in: the wrap segment
	// containing segmentStart (wrap on), else the viewport's columns.
	struct TextWindow
	{
		int from = 0;
		int to = 0;
		int visualBase = 0;
	};
	TextWindow textWindow(int row, int segmentStart = 0) const;
	RowText expandWindow(int row, int fromByte, int toByte, int visualBase) const;
	// Pixel x (from the row's text origin) of a byte column.
	qreal xAtByteColumn(int row, int byteColumn, int segmentStart = 0) const;
	// Inverse for mouse hit-testing (segmentStart for wrapped rows).
	int byteColumnAtX(int row, qreal x, int segmentStart = 0) const;
	// Visual line of a (row, byte column) caret — wrap-aware.
	int visualLineOf(int row, int column) const;
	// Byte column where the caret's wrap segment starts (tab rebase point).
	int caretSegmentStart(int row, int column) const;
};