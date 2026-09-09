/*
	File: views/qt/editor_input.h
	Description: Keyboard/mouse/IME input and hover dispatch for the
	editor — QWidget events land on EditorFrame, which forwards here.
	Counterpart of views/imgui/editor_input.h (ImGui IO polling).
*/

#pragma once

#include "../../util/hover_trigger.h"

#include <QPoint>
#include <QVariant>

#include <vector>

class EditorFrame;
class QEvent;
class QFocusEvent;
class QInputMethodEvent;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
struct DiagnosticItem;

class EditorInput
{
  public:
	explicit EditorInput(EditorFrame &frame) : frame(&frame) {}

	void keyPress(QKeyEvent *event);
	void inputMethod(QInputMethodEvent *event);
	QVariant inputQuery(Qt::InputMethodQuery query) const;
	bool event(QEvent *event);
	void focusIn(QFocusEvent *event);
	void focusOut(QFocusEvent *event);
	void wheel(QWheelEvent *event);
	void leave(QEvent *event);
	void mousePress(QMouseEvent *event);
	void mouseDoubleClick(QMouseEvent *event);
	void mouseMove(QMouseEvent *event);
	void mouseRelease(QMouseEvent *event);
	void showContextMenu(const QPoint &pos);
	HoverTrigger::Target hoverTargetAt(const QPoint &pos) const;
	void updateHover(bool mouseMoved, bool dismissed, const QPoint &pos);
	void fireHover(const HoverTrigger::Info &info);
	void showDiagnosticTooltip(const std::vector<DiagnosticItem> &items,
							   const QPoint &globalPos);
	void hideHoverTooltips();

  private:
	// Font zoom helper — member so EditorFrame friendship covers it.
	static void zoomFont(EditorFrame *frame, int delta);

	EditorFrame *frame;
};
