/*
	File: views/qt/gutter_view.h
	Description: Line-number gutter — numbers, git line marks, diagnostic severity
	marks. Qt counterpart of views/imgui/gutter_view.h.
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

class GutterView
{
  public:
	explicit GutterView(EditorFrame &frame) : frame(&frame) {}

	void updateWidth();
	int diagColumnWidth() const;
	int diffColumnWidth() const; // +/- marker column (diff views only)
	void paint(QPainter &painter,
			   int firstRow,
			   int rows,
			   qreal yBase,
			   const std::vector<int> &diagSeverity,
			   int diagColW);

  private:
	EditorFrame *frame;
};
