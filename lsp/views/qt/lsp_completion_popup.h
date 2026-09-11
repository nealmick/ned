/*
	File: views/qt/lsp_completion_popup.h
	Description: Completion list popup for the Qt backend. A focusless
	ToolTip-level window painted like the hover card (rounded, themed): the
	editor keeps keyboard focus — navigation keys arrive via LSPView's event
	filter, only mouse clicks/wheel land here. `present()` is safe to call
	repeatedly: async results refresh the open list in place, and identical
	content only repositions (the caret moves between polls).
*/

#pragma once

#include "../../../lsp/lsp_completion_model.h"

#include <QColor>
#include <QFont>
#include <QPoint>
#include <QWidget>
#include <vector>

// Per-kind badge colors (Sublime-style single letters, color-coded).
// Built by the caller from the editor's syntax theme so the badges match
// the active theme's keyword/type/function colors.
struct LSPCompletionKindColors
{
	QColor function, type, variable, keyword, number, special, op;
};

class LSPCompletionPopup : public QWidget
{
	Q_OBJECT

  public:
	explicit LSPCompletionPopup(QWidget *parent = nullptr);

	// Show (or refresh) `entries` with `selected` highlighted, anchored
	// below the caret (flipped above near the screen bottom).
	void present(const std::vector<LSPCompletionEntry> &entries,
				 int selected,
				 const QFont &font,
				 const QColor &card,
				 const QColor &ink,
				 const QColor &accent,
				 const LSPCompletionKindColors &kindColors,
				 const QPoint &globalAnchor);

	// Keyboard navigation (driven by the controller — the editor has focus).
	void moveSelection(int delta);
	void pageSelection(int deltaPages);
	int selection() const { return selected; }
	int rowCount() const { return static_cast<int>(entries.size()); }

  Q_SIGNALS:
	void itemAccepted(int index);
	void dismissed();

  protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	void hideEvent(QHideEvent *event) override;

  private:
	void ensureVisible(int row);
	int rowAt(const QPoint &pos) const;

	std::vector<LSPCompletionEntry> entries;
	int selected = 0;
	int firstVisible = 0;

	QFont rowFont;
	QColor cardColor, inkColor, accentColor;
	LSPCompletionKindColors kindColors;
	int rowHeight = 20;
	// Column layout, measured in present() from the actual entries (the
	// kind tag never overlaps the label; the detail elides into whatever
	// genuinely remains before kMaxWidth).
	int labelX = 0;
	int tagWidth = 0;
	int labelWidth = 0;
	int detailWidth = 0;
	int contentWidth = 200;

	static constexpr int kMaxVisibleRows = 8;
	static constexpr int kPadX = 10;
	static constexpr int kPadY = 6;
	static constexpr int kColGap = 12; // tag<->label and label<->detail
	static constexpr int kMinWidth = 160;
	static constexpr int kMaxWidth = 480;
};
