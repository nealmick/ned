/*
	File: views/qt/minimap_view.h
	Description: Right-strip code overview for the Qt editor (ImGui
	minimap_view parity): per-line density runs with syntax colors dimmed,
	a visible-window slider, and the strip geometry shared by painting
	and click/drag scrolling — ONE definition, because the two drifting
	apart is exactly how the old "scroll undershoots the bottom" bug
	happened.
*/

#pragma once

#include <QColor>
#include <QPointF>
#include <QString>

#include <vector>

class EditorFrame;
class EditorHighlight;
class EditorState;
class QPainter;

class MinimapView
{
  public:
	struct Run
	{
		qreal x, y, w, h;
		QColor ink;
	};

	// Strip geometry derived from (strip height, viewport lines,
	// document lines, scroll range). rowH/dotH constants match the ImGui
	// density model (~2px rows, dots at 75% row height).
	struct Geometry
	{
		qreal rowH = 2.0;
		qreal sliderH = 0.0;
		qreal maxTop = 0.0;
		qreal ratio = 0.0;
		int fit = 1;

		// Slider top for a scroll position (continuous, clamped).
		qreal sliderTopFor(qreal scrollLines) const;
		// Scroll position for a clicked/dragged strip y (continuous).
		qreal scrollLinesForY(qreal yInStrip) const;
	};

	static Geometry
	geometry(qreal stripH, int viewLines, int lineCount, int maxScrollLine);

	// Rebuild the density runs for [startRow, endRow] when the cache key
	// changed; same key = keep (scroll/selection untouched). Colors come
	// from the highlight service, dimmed to the ImGui minimap's 72%.
	void ensureRuns(const QString &key,
					int startRow,
					int endRow,
					EditorState &state,
					EditorHighlight &highlight,
					qreal stripTop,
					qreal dotH,
					qreal padX,
					qreal charW,
					int maxCols);

	const std::vector<Run> &runs() const { return runs_; }

	// --- EditorFrame seam (ImGui MinimapView parity: paint + interact) ---
	// paint draws the strip inside the editor view (called from
	// EditorFrame::paintEvent). interact entry points take widget-space
	// positions; press returns true when the click landed in the strip and
	// was consumed, move only acts while a strip drag is active.
	void paint(QPainter &painter, EditorFrame &frame);
	bool press(EditorFrame &frame, const QPointF &pos);
	void move(EditorFrame &frame, const QPointF &pos);
	void release();
	bool dragging() const { return dragging_; }

  private:
	void scrollTo(EditorFrame &frame, qreal y);

	QString key;
	std::vector<Run> runs_;
	bool dragging_ = false;
};
