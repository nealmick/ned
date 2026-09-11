#include "editor_input.h"

#include "diagnostic_style.h"
#include "editor_frame.h"

#include "../../services/diagnostics/diagnostic_colors.h"
#include "../../util/text_columns.h"
#include "../../util/utf8.h"

#include "../../../util/settings.h"
#include "find_bar.h"
#include "hover_tooltip.h"
#include "line_jump.h"
#include "ned_color.h"

#include "../../../../host/qt/qt_icons.h"
#include "host/qt/fonts.h"
#include "host/qt/theme.h"
#include <QApplication>
#include <QFontMetrics>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTimer>
#include <QWheelEvent>

#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>

// Font zoom (Cmd +/-): bump the persisted size, apply, notify, save.
void EditorInput::zoomFont(EditorFrame *frame, int delta)
{
	if (delta >= 0)
		frame->appSettings.settings["fontSize"] =
			frame->appSettings.settings.value("fontSize", 13) + delta;
	else
		frame->appSettings.settings["fontSize"] =
			std::max(8.0,
					 frame->appSettings.settings.value("fontSize", 13) +
						 static_cast<double>(delta));
	frame->applyProfileFont();
	Q_EMIT frame->fontZoomed();
	frame->appSettings.saveSettings();
}

void EditorInput::inputMethod(QInputMethodEvent *event)
{
	// IME preedit renders inline as hollow text; commit goes through the
	// normal typing path (CJK input, dead keys, dictation).
	if (!event->commitString().isEmpty() && !frame->readOnly())
	{
		frame->commands.typeText(event->commitString().toUtf8().constData());
		frame->afterEdit();
	}
	frame->setAttribute(Qt::WA_InputMethodEnabled);
	frame->update();
}

QVariant EditorInput::inputQuery(Qt::InputMethodQuery query) const
{
	if (query == Qt::ImEnabled)
		return true;
	return frame->QWidget::inputMethodQuery(query);
}

HoverTrigger::Target EditorInput::hoverTargetAt(const QPoint &pos) const
{
	HoverTrigger::Target target;
	if (pos.y() < frame->topInset() || pos.y() > frame->height() || pos.x() < 0)
		return target;
	if (pos.x() >= frame->width() - frame->minimapWidth())
		return target; // minimap strip is not a hover zone
	if (pos.x() < frame->gutterWidthPx)
	{
		target.zone = HoverTrigger::Zone::Gutter;
		target.row = frame->rowAtY(pos.y());
		return target;
	}
	const EditorFrame::EditorFrame::RowHit hit = frame->hitTestY(pos.y());
	// Past-end-of-text guard: columnAtX snaps to the nearest glyph and
	// clamps to the line length, so a mouse far right of the last glyph
	// would still report the end-of-line column — and LSP servers answer
	// that with the last token's hover. Only count cells actually on the
	// rendered text (half a char of slack for the last glyph's edge).
	const int seg = frame->wrap.segmentOf(hit.row, hit.segmentStart);
	const int segEnd = seg + 1 < frame->wrap.segmentCount(hit.row)
						   ? frame->wrap.segmentStartColumn(hit.row, seg + 1)
						   : frame->state.lineLength(hit.row);
	// xAtByteColumn is widget-space — compare against the mouse directly.
	const qreal textEndX = frame->xAtByteColumn(hit.row, segEnd, hit.segmentStart);
	if (static_cast<qreal>(pos.x()) > textEndX + frame->charWidthF() * 0.5)
		return target;
	target.zone = HoverTrigger::Zone::Text;
	target.row = hit.row;
	target.column = frame->columnAtX(hit.row, pos.x(), hit.segmentStart);
	return target;
}

void EditorInput::updateHover(bool mouseMoved, bool dismissed, const QPoint &pos)
{
	const HoverTrigger::Info prev = frame->liveHoverInfo;
	const HoverTrigger::Target target =
		(dismissed || frame->dragging || !frame->underMouse()) ? HoverTrigger::Target{}
															   : hoverTargetAt(pos);
	frame->hoverTrigger.update(mouseMoved, dismissed, target);
	frame->lastHoverPos = pos;

	const HoverTrigger::Info current = frame->hoverTrigger.info();
	// Only state transitions act: arm→fire, active→inactive (dismiss),
	// retarget. Both-inactive ticks (the common case) do nothing.
	const bool unchanged =
		current.active == prev.active &&
		(!current.active || (current.row == prev.row && current.column == prev.column &&
							 current.zone == prev.zone));
	if (unchanged)
		return;
	frame->liveHoverInfo = current;
	if (current.active)
		fireHover(current);
	else
		hideHoverTooltips();
}

void EditorInput::fireHover(const HoverTrigger::Info &info)
{
	if (frame->state.path.empty())
		return;

	// Diagnostics own the tooltip first (ImGui: squiggle/gutter claims win
	// over symbol hover) — Gutter zones match by row, Text by exact cell.
	if (frame->diagStore)
	{
		std::vector<DiagnosticItem> matched;
		if (info.zone == HoverTrigger::Zone::Gutter)
		{
			matched = frame->diagStore->forLine(frame->state.path, info.row);
		} else if (info.zone == HoverTrigger::Zone::Text)
		{
			const int utf16 = EditorUtils::Utf8ByteOffsetToUtf16(
				frame->state.line(info.row), info.column);
			for (const DiagnosticItem &d :
				 frame->diagStore->forLine(frame->state.path, info.row))
				if (DiagnosticContains(d, info.row, utf16))
					matched.push_back(d);
		}
		if (!matched.empty())
		{
			showDiagnosticTooltip(matched, QCursor::pos());
			return;
		}
	}

	// No diagnostic there: Text cells fall through to the symbol hover.
	if (info.zone == HoverTrigger::Zone::Text && frame->hoverObserver)
		frame->hoverObserver(info);
}

void EditorInput::showDiagnosticTooltip(const std::vector<DiagnosticItem> &items,
										const QPoint &globalPos)
{
	if (!frame->diagTip)
		frame->diagTip = new HoverTooltip(frame);

	// Severity card: colored dot + bold label (+ dim source), message
	// below, items separated by hairlines.
	const QColor ink = toQColor(frame->highlight.defaultTextColor());
	QString html;
	for (size_t i = 0; i < items.size(); ++i)
	{
		if (i)
			html += "<hr>";
		const DiagnosticSeverityRGB c = DiagnosticSeverityColor(items[i].severity);
		const QColor sevColor = QColor::fromRgbF(c.r, c.g, c.b);
		html += QString("<b style=\"color:%1\">● %2</b>")
					.arg(sevColor.name(),
						 QString::fromUtf8(DiagnosticSeverityLabel(items[i].severity)));
		if (!items[i].source.empty())
			html += QStringLiteral("&nbsp;&nbsp;<span style=\"color:#888888\">") +
					QString::fromStdString(items[i].source).toHtmlEscaped() + "</span>";
		html += "<br>" + QString::fromStdString(items[i].message).toHtmlEscaped();
	}
	frame->diagTip->present(html,
							frame->font(),
							NedQtTheme::raised(NedQtTheme::background(frame->appSettings)),
							ink,
							globalPos);
}

void EditorInput::hideHoverTooltips()
{
	if (frame->diagTip)
		frame->diagTip->hide();
	if (frame->hoverObserver)
		frame->hoverObserver(HoverTrigger::Info{});
}

void EditorInput::focusIn(QFocusEvent *event)
{
	frame->QWidget::focusInEvent(event);
	// Regained focus: solid caret, blink phase restarts.
	frame->blinkClock.restart();
	frame->caretVisible = true;
	frame->update();
}

void EditorInput::focusOut(QFocusEvent *event)
{
	frame->QWidget::focusOutEvent(event);
	// Repaint now so an unfocused dock sibling's caret disappears immediately
	// instead of lingering until the next blink flip.
	frame->update();
}

void EditorInput::keyPress(QKeyEvent *event)
{
	const bool shift = event->modifiers() & Qt::ShiftModifier;
	const bool ctrl = event->modifiers() & Qt::ControlModifier;
	const bool meta = event->modifiers() & Qt::MetaModifier; // Cmd
	const bool alt = event->modifiers() & Qt::AltModifier;	 // Option
	const bool primary = ctrl || meta;

	// Diff views are read-only: navigation and copy/select still work;
	// every mutating keystroke is swallowed. (Selection/caret movement is
	// harmless — there is no way to dirty the buffer from here.)
	if (frame->readOnly())
	{
		switch (event->key())
		{
		case Qt::Key_Left:
		case Qt::Key_Right:
		case Qt::Key_Up:
		case Qt::Key_Down:
		case Qt::Key_Home:
		case Qt::Key_End:
		case Qt::Key_PageUp:
		case Qt::Key_PageDown:
			break; // fall through to the normal handling below
		case Qt::Key_A:
		case Qt::Key_C:
			if (primary)
				break;
			return;
		default:
			return;
		}
	}

	// Any keystroke dismisses hover popups (HoverTrigger rule 2).
	updateHover(false, true, frame->lastHoverPos);

	// Option/Alt: word left/right, add caret above/below (not with Cmd/Ctrl).
	if (alt && !primary)
	{
		switch (event->key())
		{
		case Qt::Key_Left:
			frame->commands.moveWordLeft(shift);
			break;
		case Qt::Key_Right:
			frame->commands.moveWordRight(shift);
			break;
		case Qt::Key_Up:
			frame->commands.addCursorAbove();
			break;
		case Qt::Key_Down:
			frame->commands.addCursorBelow();
			break;
		case Qt::Key_Backspace:
			frame->commands.deleteLeft(true); // Alt deletes by word (ImGui: KeyAlt)
			break;
		case Qt::Key_Delete:
			frame->commands.deleteRight(true);
			break;
		default:
			frame->QWidget::keyPressEvent(event);
			return;
		}
		frame->afterEdit();
		return;
	}

	if (primary)
	{
		switch (event->key())
		{
		case Qt::Key_A:
			frame->commands.selectAll();
			break;
		case Qt::Key_Plus:
		case Qt::Key_Equal:
			zoomFont(frame, 2);
			break;
		case Qt::Key_Minus:
			zoomFont(frame, -2);
			break;
		case Qt::Key_Z:
			frame->commands.undo();
			break;
		case Qt::Key_Y:
			frame->commands.redo();
			break;
		case Qt::Key_C:
			frame->commands.copy();
			break;
		case Qt::Key_X:
			frame->commands.cut();
			break;
		case Qt::Key_V:
			frame->commands.paste();
			break;
		case Qt::Key_S:
			frame->commands.save();
			break;
		case Qt::Key_Left:
			frame->commands.moveLineStart(shift);
			break;
		case Qt::Key_Right:
			frame->commands.moveLineEnd(shift);
			break;
		case Qt::Key_Up:
			frame->commands.moveLines(-5, shift);
			break;
		case Qt::Key_Down:
			frame->commands.moveLines(5, shift);
			break;
		default:
			frame->QWidget::keyPressEvent(event);
			return;
		}
		frame->afterEdit();
		return;
	}

	switch (event->key())
	{
	case Qt::Key_Left:
		frame->commands.moveLeft(shift);
		break;
	case Qt::Key_Right:
		frame->commands.moveRight(shift);
		break;
	case Qt::Key_Up:
		frame->commands.moveUp(shift);
		break;
	case Qt::Key_Down:
		frame->commands.moveDown(shift);
		break;
	case Qt::Key_Home:
		frame->commands.moveLineStart(shift);
		break;
	case Qt::Key_End:
		frame->commands.moveLineEnd(shift);
		break;
	case Qt::Key_Escape:
		// Find bar open: Escape closes it before anything else (the bar
		// itself handles Escape when its input has focus). A QShortcut
		// was tried here — its WidgetWithChildren context also swallowed
		// Escape for child popups like the line jump card.
		if (frame->findBar->isVisible())
			frame->closeFindBar();
		else
			frame->commands.collapseSelection();
		break;
	case Qt::Key_Return:
	case Qt::Key_Enter:
		frame->commands.insertNewline();
		break;
	case Qt::Key_Backspace:
		frame->commands.deleteLeft(); // word-delete variant lives in the Alt branch
		break;
	case Qt::Key_Delete:
		frame->commands.deleteRight();
		break;
	case Qt::Key_Tab:
		frame->commands.indent();
		break;
	case Qt::Key_Backtab:
		frame->commands.outdent();
		break;
	default: {
		const QString text = event->text();
		if (!text.isEmpty())
		{
			frame->commands.typeText(text.toUtf8().constData());
			break;
		}
		frame->QWidget::keyPressEvent(event);
		return;
	}
	}
	frame->afterEdit();
}

void EditorInput::wheel(QWheelEvent *event)
{
	// ImGui parity (editor_view_scroll.cpp): 120 delta units = 3 lines —
	// accumulated fractionally in PIXELS so trackpad micro-deltas and
	// momentum scroll with sub-line smoothness. The axes apply
	// INDEPENDENTLY: a diagonal trackpad gesture scrolls both. (The old
	// "any x-delta routes the whole event horizontal" rule ate the
	// vertical half of angled swipes — the wheel felt dead, e.g. trying
	// to scroll back up from the bottom of a file.)
	const bool shift = event->modifiers() & Qt::ShiftModifier;
	const bool wrapping = frame->wordWrapEnabled();
	const QPoint d = event->angleDelta();

	// Horizontal (wrap off): x-deltas; shift maps the y-wheel to horizontal
	// on platforms that deliver shift+wheel unswapped (macOS/Qt pre-swap it
	// into the x axis already). Same sign convention as the vertical axis —
	// content follows the gesture. 120 units = 3 cells.
	if (!wrapping)
	{
		const int dx = d.x() != 0 ? d.x() : (shift ? d.y() : 0);
		if (dx != 0)
		{
			frame->viewState.wheelCarryX -= dx * (3.0 * frame->charWidthF() / 120.0);
			const int px = static_cast<int>(frame->viewState.wheelCarryX);
			frame->viewState.wheelCarryX -= px;
			if (px != 0)
				frame->setScrollXPixels(frame->viewState.scrollPxX + px);
		}
	}

	// Vertical: the y axis. Shift means horizontal intent on unswapped
	// wheels, so skip it there — unless wrapping, where ImGui's rule
	// scrolls vertically regardless.
	if (d.y() != 0 && (!shift || wrapping))
	{
		frame->viewState.wheelCarry += d.y() * (3.0 * frame->lineHeightPx / 120.0);
		const int px = static_cast<int>(frame->viewState.wheelCarry);
		frame->viewState.wheelCarry -= px;
		if (px != 0)
			frame->setScrollPixels(frame->viewState.scrollPx - px);
	}
	// Scrolling shifts content under the mouse — dismiss hover (rule 2).
	updateHover(false, true, frame->lastHoverPos);
}

void EditorInput::leave(QEvent *event)
{
	frame->QWidget::leaveEvent(event);
	// No zone outside the widget (the mouse may be resting ON a hover
	// tooltip — its own dismissal paths handle that case).
	updateHover(false, false, QPoint(-1, -1));
}

void EditorInput::mousePress(QMouseEvent *event)
{
	// Click/drag is a hover dismissal (rule 2) — before anything else.
	updateHover(false, true, frame->lastHoverPos);

	if (event->button() == Qt::RightButton)
	{
		showContextMenu(event->pos());
		return;
	}
	if (frame->minimapView.press(*frame, event->position()))
		return;
	if (event->button() != Qt::LeftButton)
		return;
	const EditorFrame::EditorFrame::RowHit hit =
		frame->hitTestY(static_cast<int>(event->position().y()));
	const int row = hit.row;
	const int column =
		frame->columnAtX(row, static_cast<int>(event->position().x()), hit.segmentStart);
	frame->dragging = true;
	frame->caretVisible = true;
	frame->scheduleBlink();
	if (event->modifiers() & Qt::ShiftModifier)
	{
		// Extend from the existing anchor (ImGui handleMouseClick).
		const Selection &p = frame->viewState.selections[frame->viewState.primaryIndex];
		frame->commands.setSelection(p.anchorRow, p.anchorColumn, row, column);
	} else
	{
		frame->commands.setCursor(row, column, false);
	}
	frame->update();
}

void EditorInput::mouseDoubleClick(QMouseEvent *event)
{
	if (event->button() != Qt::LeftButton)
		return;
	// Qt fires the double-click INSTEAD of a second mousePressEvent — arm
	// dragging here too, or click-click-drag would never extend the
	// selection (setCursor select=true keeps the word anchor).
	frame->dragging = true;
	frame->caretVisible = true;
	frame->scheduleBlink();
	const EditorFrame::EditorFrame::RowHit hit =
		frame->hitTestY(static_cast<int>(event->position().y()));
	const int column = frame->columnAtX(
		hit.row, static_cast<int>(event->position().x()), hit.segmentStart);
	frame->commands.selectWordAt(hit.row, column);
	frame->update();
}

void EditorInput::showContextMenu(const QPoint &pos)
{
	QMenu menu(frame);
	// Standard keys so the accelerators render platform-native (⌘ on macOS).
	menu.addAction(
		"Cut",
		[this] {
			frame->commands.cut();
			frame->afterEdit();
		},
		QKeySequence(QKeySequence::Cut));
	menu.addAction(
		"Copy", [this] { frame->commands.copy(); }, QKeySequence(QKeySequence::Copy));
	menu.addAction(
		"Paste",
		[this] {
			frame->commands.paste();
			frame->afterEdit();
		},
		QKeySequence(QKeySequence::Paste));
	menu.addSeparator();
	menu.addAction(
		"Select All",
		[this] {
			frame->commands.selectAll();
			frame->update();
		},
		QKeySequence(QKeySequence::SelectAll));
	menu.exec(frame->mapToGlobal(pos));
}

void EditorInput::mouseMove(QMouseEvent *event)
{
	// Real mouse movement (re)arms the hover delay; a drag doubles as a
	// dismissal signal — target resolution in updateHover drops the zone.
	updateHover(true, frame->dragging, event->position().toPoint());

	// Heal drags whose release was consumed elsewhere (context-menu nested
	// loop, popup, window deactivate): with the left button up there is no
	// drag. A stuck minimap drag re-pinned the scroll to the pointer on
	// every move — "stuck at the bottom, can't scroll up".
	if ((frame->dragging || frame->minimapView.dragging()) &&
		!(QGuiApplication::mouseButtons() & Qt::LeftButton))
	{
		frame->dragging = false;
		frame->minimapView.release();
		return;
	}

	if (frame->minimapView.dragging())
	{
		frame->minimapView.move(*frame, event->position());
		return;
	}
	if (!frame->dragging)
		return;
	const EditorFrame::EditorFrame::RowHit hit =
		frame->hitTestY(static_cast<int>(event->position().y()));
	const int column = frame->columnAtX(
		hit.row, static_cast<int>(event->position().x()), hit.segmentStart);
	frame->commands.setCursor(hit.row, column, true);
	frame->update();
}

void EditorInput::mouseRelease(QMouseEvent *)
{
	frame->dragging = false;
	frame->minimapView.release();
}

int EditorFrame::rowAtY(int y) const
{
	const qreal v = (viewState.scrollPx + std::max(0, y - topInset())) / lineHeightPx;
	if (wordWrapEnabled())
	{
		// v is CONTINUOUS (the pointer can be anywhere within a visual
		// line) — yToRow floors it directly. An extra +0.5 here pushed
		// everything in a row's lower half down to the next visual line.
		const WrapLayout::Hit hit = wrap.yToRow(static_cast<float>(v));
		return std::clamp(hit.row, 0, std::max(0, state.lineCount() - 1));
	}
	return std::clamp(static_cast<int>(v), 0, std::max(0, state.lineCount() - 1));
}

int EditorFrame::columnAtX(int row, int x, int segmentStart) const
{
	if (wordWrapEnabled())
	{
		// ImGui parity (editor_input.cpp rowColFromMouse): map through the
		// shared wrap layout so continuation segments resolve against their
		// own start column — the monospace byte-map below knows nothing
		// about segments and lands clicks on the wrong columns.
		const std::string line = state.line(row);
		if (line.empty())
			return 0;
		const int seg = wrap.segmentOf(row, segmentStart);
		const int column = wrap.columnAt(line, row, seg, x - gutterWidthPx);
		return EditorUtils::snapToUtf8CharBoundary(line, column);
	}
	// Wrap off: x is screen space — shift by the horizontal scroll offset.
	const int textX = x - gutterWidthPx + static_cast<int>(viewState.scrollPxX);
	if (textX <= 0)
		return segmentStart;
	return std::clamp(
		byteColumnAtX(row, static_cast<qreal>(textX), 0), 0, state.lineLength(row));
}

// y -> document row + wrap-segment start (byte column of the segment).
EditorFrame::RowHit EditorFrame::hitTestY(int y) const
{
	const qreal v = (viewState.scrollPx + std::max(0, y - topInset())) / lineHeightPx;
	if (wordWrapEnabled())
	{
		// v is CONTINUOUS (a click lands anywhere within a visual line), so
		// yToRow floors it directly — the extra +0.5 here used to resolve
		// clicks in a row's lower half onto the NEXT visual line, putting
		// the caret a few columns past the click on wrapped rows.
		const WrapLayout::Hit hit = wrap.yToRow(static_cast<float>(v));
		RowHit out;
		out.row = std::clamp(hit.row, 0, std::max(0, state.lineCount() - 1));
		out.segmentStart = wrap.segmentStartColumn(out.row, hit.segment);
		return out;
	}
	return {std::clamp(static_cast<int>(v), 0, std::max(0, state.lineCount() - 1)), 0};
}

bool EditorFrame::event(QEvent *event)
{
	// Qt routes Tab through focus traversal before keyPressEvent; claim it
	// first so the editor always receives it.
	if (event->type() == QEvent::KeyPress)
	{
		auto *key = static_cast<QKeyEvent *>(event);
		if (key->key() == Qt::Key_Tab || key->key() == Qt::Key_Backtab)
		{
			keyPressEvent(key);
			return true;
		}
	}
	return QWidget::event(event);
}

// --- QWidget event plumbing: forward into the EditorInput component ---

void EditorFrame::inputMethodEvent(QInputMethodEvent *event) { input.inputMethod(event); }

QVariant EditorFrame::inputMethodQuery(Qt::InputMethodQuery query) const
{
	return input.inputQuery(query);
}

void EditorFrame::focusInEvent(QFocusEvent *event) { input.focusIn(event); }

void EditorFrame::focusOutEvent(QFocusEvent *event) { input.focusOut(event); }

void EditorFrame::keyPressEvent(QKeyEvent *event) { input.keyPress(event); }

void EditorFrame::wheelEvent(QWheelEvent *event) { input.wheel(event); }

void EditorFrame::leaveEvent(QEvent *event) { input.leave(event); }

void EditorFrame::mousePressEvent(QMouseEvent *event) { input.mousePress(event); }

void EditorFrame::mouseDoubleClickEvent(QMouseEvent *event)
{
	input.mouseDoubleClick(event);
}

void EditorFrame::showContextMenu(const QPoint &pos) { input.showContextMenu(pos); }

void EditorFrame::mouseMoveEvent(QMouseEvent *event) { input.mouseMove(event); }

void EditorFrame::mouseReleaseEvent(QMouseEvent *event) { input.mouseRelease(event); }
