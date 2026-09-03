/*
	File: views/qt/caret_view.h
	Description: Caret + selection painting on the shared tab-expanded coordinate
	space. Qt counterpart of views/imgui/caret_view.h.
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

class CaretView
{
  public:
	explicit CaretView(EditorFrame &frame) : frame(&frame) {}

	// Pixel position of the primary caret (widget coords); LSP hover
	// anchors its tooltip here (ImGui caretScreenX parity).
	QPoint caretWidgetPos() const;
	void paint(QPainter &painter, int firstRow, int rows, qreal yBase, qreal textX0);

  private:
	EditorFrame *frame;
};
