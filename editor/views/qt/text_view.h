/*
	File: views/qt/text_view.h
	Description: Document text painting — syntax colors through the tab-expanded
	model, plus diagnostic squiggles. Qt counterpart of
	views/imgui/text_view.h.
*/

#pragma once

#include <QColor>
#include <QPainter>
#include <QPoint>
#include <QString>
#include <QVariant>

#include <string>
#include <vector>

struct DiagnosticItem;
class EditorFrame;
class QEvent;
class QFocusEvent;
class QInputMethodEvent;
class QKeyEvent;
class QMouseEvent;
class QPainter;
class QWheelEvent;
struct HoverTrigger;

class TextView
{
  public:
	explicit TextView(EditorFrame &frame) : frame(&frame) {}

	// Paints the text pass (viewport byte windows only; widget-space x).
	void paint(QPainter &painter, int firstRow, int rows, qreal yBase);
	void paintDiagnosticSquiggles(QPainter &painter,
								  int firstVisual,
								  int visualRows,
								  qreal yBase);

  private:
	void paintRow(QPainter &painter,
				  int row,
				  qreal y,
				  int fromByte,
				  int toByte,
				  int visualBase,
				  qreal textLeft);
	void
	paintSquiggle(QPainter &painter, qreal x0, qreal x1, qreal y, const QColor &color);

  private:
	EditorFrame *frame;
};
