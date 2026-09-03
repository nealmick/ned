/*
	File: views/qt/title_bar_view.h
	Description: Editor title strip — file icon, full path, git ±N. Qt counterpart
	of views/imgui/title_bar_view.h (painting + icon reload).
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

class TitleBarView
{
  public:
	explicit TitleBarView(EditorFrame &frame) : frame(&frame) {}

	void paint(QPainter &painter);
	std::string gitSummary() const;
	void reloadIcon();

  private:
	EditorFrame *frame;
};
