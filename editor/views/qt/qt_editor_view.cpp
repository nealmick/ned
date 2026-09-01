#include "qt_editor_view.h"

#include "../../util/utf8.h"
#include "../../../util/settings.h"
#include "ned_color_qt.h"
#include "qt_find_bar.h"

#include <QShortcut>

#include <QFontMetrics>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTimer>
#include "qt_icons.h"
#include <QWheelEvent>

#include <cmath>
#include <fstream>
#include <sstream>

QtEditorView::QtEditorView(Settings &settings, QWidget *parent)
	: QWidget(parent), appSettings(settings), projectUndo(projectRoot), state(),
	  events(), ops(state), viewState(state), save(state, events),
	  highlight(state, ops, &settings), git(state, projectRoot, appSettings),
	  commands(state, viewState, ops, projectUndo, events, save)
{
	setFontFromSettings();

	blinkTimer = new QTimer(this);
	blinkTimer->setInterval(530);
	// Blink state is computed in the service timer; this timer restarts the
	// blink phase whenever typing/moving keeps the caret "alive".
	connect(blinkTimer, &QTimer::timeout, this, [this] {
		blinkClock.restart();
		caretVisible = true;
	});
	blinkTimer->start();

	scrollBar = new QScrollBar(Qt::Vertical, this);
	connect(scrollBar, &QScrollBar::valueChanged, this,
			[this](int) { update(); });

	// Services (async tree-sitter, autosave, git status) expect per-frame
	// polling; the Qt backend has no frame loop, so a short timer drives
	// them — plus smooth caret blink/rainbow animation.
	serviceTimer = new QTimer(this);
	serviceTimer->setInterval(30);
	blinkClock.start();
	connect(serviceTimer, &QTimer::timeout, this, [this] {
		highlight.poll();
		git.poll();
		if (highlight.visualGeneration() != lastVisualGen)
		{
			lastVisualGen = highlight.visualGeneration();
			update();
		}
		// Blink at ~1.9 Hz; rainbow hue cycles continuously.
		caretVisible = (blinkClock.elapsed() % 1060) < 530;
		++rainbowPhase;
		if (rainbowMode())
			update();
	});
	serviceTimer->start();

	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);

	findBar = new QtFindBar(this, this);
	auto *lineJumpShortcut = new QShortcut(QKeySequence("Ctrl+;"), this);
	connect(lineJumpShortcut, &QShortcut::activated, this,
			&QtEditorView::goToLineDialog);
	auto *findShortcut = new QShortcut(QKeySequence("Ctrl+F"), this);
	connect(findShortcut, &QShortcut::activated, this, &QtEditorView::toggleFindBar);
}

QtEditorView::~QtEditorView() = default;

void QtEditorView::inputMethodEvent(QInputMethodEvent *event)
{
	// IME preedit renders inline as hollow text; commit goes through the
	// normal typing path (CJK input, dead keys, dictation).
	if (!event->commitString().isEmpty())
	{
		commands.typeText(event->commitString().toUtf8().constData());
		afterEdit();
	}
	setAttribute(Qt::WA_InputMethodEnabled);
	update();
}

QVariant QtEditorView::inputMethodQuery(Qt::InputMethodQuery query) const
{
	if (query == Qt::ImEnabled)
		return true;
	return QWidget::inputMethodQuery(query);
}

void QtEditorView::openWorkspaceRoot(const std::string &root)
{
	projectRoot = root;
	git.init();
}

void QtEditorView::setFontFromSettings()
{
	QFont font("Menlo"); // fallback; profile font wins when registered
#ifdef _WIN32
	font.setFamily("Consolas");
#endif
	// Profile font (registered from resources/fonts by the settings dialog).
	const std::string profileFont =
		appSettings.settings.value("font", std::string());
	if (!profileFont.empty() && profileFont != "System Default")
	{
		font.setFamily(QString::fromStdString(profileFont));
		font.setFixedPitch(true);
	}
	font.setStyleHint(QFont::Monospace);
	font.setFixedPitch(true);
	font.setPointSize(
		static_cast<int>(appSettings.settings.value("fontSize", 13)));
	setFont(font);

	const QFontMetrics metrics(font);
	lineHeightPx = metrics.height();
	// Monospace advance: '0' is reliably full-width; ' ' can be narrower.
	charWidthPx = metrics.horizontalAdvance(QLatin1String("0000")) / 4.0;
	gutterWidthPx = metrics.horizontalAdvance('0') * 6 + 16;
}

void QtEditorView::openFile(const QString &path)
{
	std::string raw;
	if (!path.isEmpty())
	{
		std::ifstream file(path.toStdString(), std::ios::binary);
		std::stringstream buffer;
		buffer << file.rdbuf();
		raw = buffer.str();
		state.path = path.toStdString();
		state.languageId = EditorState::languageIdFromPath(state.path);
	} else
	{
		state.path = "";
	}

	fileIcon = QtIconSet::instance().forFile(path);
	state.setFromString(raw);
	ops.clearPending();
	ops.bumpGeneration();
	viewState.setBoth(0, 0);
	highlight.resetForDocument(static_cast<size_t>(state.lineCount()));
	highlight.highlightContent();
	git.init();
	git.onDocumentOpened();
	lastVisualGen = highlight.visualGeneration();
	scrollBar->setRange(0, maxScrollLine());
	scrollBar->setValue(0);
	update();
}

QSize QtEditorView::sizeHint() const { return QSize(800, 600); }

int QtEditorView::visibleLines() const
{
	return std::max(1, (height() - titleBarPx) / lineHeightPx);
}

int QtEditorView::maxScrollLine() const
{
	// Last line fully visible at bottom: allow scrolling past it a little
	// (ImGui scrolls to keep the caret line plus context visible).
	return std::max(0, state.lineCount() - visibleLines() + 2);
}

int QtEditorView::rowAtY(int y) const
{
	int row = scrollBar->value() + std::max(0, y - titleBarPx) / lineHeightPx;
	return std::clamp(row, 0, std::max(0, state.lineCount() - 1));
}

int QtEditorView::columnAtX(int row, int x) const
{
	const int textX = x - gutterWidthPx;
	if (textX <= 0)
		return 0;
	const int cols = textX / charWidthPx;
	return std::clamp(cols, 0, state.lineLength(row));
}

void QtEditorView::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	// Background follows the active settings profile when present.
	QColor background(0x1e, 0x1e, 0x1e);
	if (appSettings.settings.contains("backgroundColor") &&
		appSettings.settings["backgroundColor"].size() >= 3)
	{
		const auto &bg = appSettings.settings["backgroundColor"];
		background = QColor::fromRgbF(bg[0].get<float>(), bg[1].get<float>(),
									  bg[2].get<float>());
	}
	painter.fillRect(rect(), background);

	const int firstRow = scrollBar->value();
	const int rows = std::min(visibleLines() + 2,
							  state.lineCount() - firstRow);

	// Editor title bar: file icon, full path, git ±N (ImGui title-bar parity).
	if (!state.path.empty())
	{
		painter.fillRect(0, 0, width(), titleBarPx, QColor(0x24, 0x24, 0x2c));
		painter.setPen(QColor(0x9a, 0x9a, 0xa8));
		QFont small = font();
		small.setPointSize(std::max(9, font().pointSize() - 3));
		painter.setFont(small);
		int tx = 10;
		if (!fileIcon.isNull())
		{
			painter.drawPixmap(QRect(tx, (titleBarPx - 16) / 2, 16, 16),
							   fileIcon.pixmap(16, 16));
			tx += 24;
		}
		painter.drawText(QRect(tx, 0, width() - tx - 160, titleBarPx),
						 Qt::AlignVCenter | Qt::AlignLeft,
						 QString::fromStdString(state.path));
		const std::string changes = git.currentGitChanges;
		if (!changes.empty())
		{
			painter.setPen(QColor(0x3f, 0xc1, 0x8c));
			painter.drawText(QRect(width() - 150, 0, 140, titleBarPx),
							 Qt::AlignVCenter | Qt::AlignRight,
							 QString::fromStdString(changes));
		}
	}

	// Gutter + current-line highlight.
	const Selection &primary = viewState.selections[viewState.primaryIndex];
	painter.setFont(font());
	for (int i = 0; i < rows; ++i)
	{
		const int row = firstRow + i;
		const int y = titleBarPx + i * lineHeightPx;
		if (row == primary.headRow)
			painter.fillRect(0, y, width(), lineHeightPx, QColor(0x2a, 0x2a, 0x2a));

		// Changed lines tint their number (green) like the ImGui gutter.
		if (git.isLineEdited(state.path, row + 1))
			painter.setPen(QColor(0x3f, 0xc1, 0x8c));
		else
			painter.setPen(QColor(0x88, 0x88, 0x88));
		painter.drawText(QRect(0, y, gutterWidthPx - 12, lineHeightPx),
						 Qt::AlignVCenter | Qt::AlignRight,
						 QString::number(row + 1));
	}

	// Git gutter marks (added/edited lines vs HEAD).
	const std::string docPath = state.path;
	for (int i = 0; i < rows; ++i)
	{
		const int row = firstRow + i;
		if (git.isLineEdited(docPath, row + 1))
			painter.fillRect(gutterWidthPx - 6, titleBarPx + i * lineHeightPx, 3, lineHeightPx,
							 QColor(0x3f, 0xc1, 0x8c));
	}

	// Text with syntax colors; spans are half-open byte ranges.
	const int textLeft = gutterWidthPx;
	const QColor defaultInk = toQColor(highlight.defaultTextColor());
	for (int i = 0; i < rows; ++i)
	{
		const int row = firstRow + i;
		const std::string text = state.line(row);
		if (text.empty())
			continue;
		const int y = titleBarPx + i * lineHeightPx;
		const LineColorSpans &spans = highlight.spansForLine(row);

		int bytePos = 0;
		QColor ink = defaultInk;
		const QFontMetrics metrics = painter.fontMetrics();
		const auto drawSegment = [&](int nextByte) {
			if (nextByte <= bytePos)
				return;
			const QByteArray seg(text.data() + bytePos, nextByte - bytePos);
			// x from the prefix advance so caret/selection/text agree exactly.
			const int x = textLeft + metrics.horizontalAdvance(
				QString::fromUtf8(text.data(), bytePos));
			painter.setPen(ink);
			painter.drawText(x, y, metrics.horizontalAdvance(
									 QString::fromUtf8(text.data(), nextByte)) -
										(x - textLeft),
							 lineHeightPx, Qt::AlignVCenter,
							 QString::fromUtf8(seg));
			bytePos = nextByte;
		};

		for (const ColorSpan &span : spans)
		{
			if (span.start > bytePos)
			{
				ink = defaultInk;
				drawSegment(span.start);
			}
			ink = toQColor(highlight.colorForSlot(span.slot));
			drawSegment(span.end);
		}
		ink = defaultInk;
		drawSegment(static_cast<int>(text.size()));
	}

	// Selections then carets.
	for (const Selection &sel : viewState.selections)
	{
		int sr, sc, er, ec;
		sel.getOrdered(sr, sc, er, ec);
		if (er < firstRow || sr > firstRow + rows)
			continue;
		painter.setPen(Qt::transparent);
		painter.setBrush(QColor(255, 30, 170, 70));
		for (int row = std::max(sr, firstRow); row <= std::min(er, firstRow + rows - 1);
			 ++row)
		{
			const int i = row - firstRow;
			const int from = row == sr ? sc : 0;
			const int to = row == er ? ec : state.lineLength(row);
			painter.drawRect(textLeft + from * charWidthPx, titleBarPx + i * lineHeightPx,
							 (to - from) * charWidthPx, lineHeightPx);
		}
	}

	if (rainbowMode() || caretVisible)
	{
		if (rainbowMode())
		{
			const float hue = std::fmod(blinkClock.elapsed() * 0.00025f, 1.0f);
			painter.setPen(QColor::fromHsvF(hue, 0.85f, 1.0f));
		} else
		{
			painter.setPen(QColor(255, 255, 255));
		}
		for (const Selection &sel : viewState.selections)
		{
			const int i = sel.headRow - firstRow;
			if (i < 0 || i >= rows + 1)
				continue;
			painter.drawLine(textLeft + sel.headColumn * charWidthPx,
							 titleBarPx + i * lineHeightPx + 2,
							 textLeft + sel.headColumn * charWidthPx,
							 titleBarPx + (i + 1) * lineHeightPx - 2);
		}
	}
}

void QtEditorView::afterEdit()
{
	highlight.poll();
	highlight.highlightContent();
	scrollBar->setRange(0, maxScrollLine());
	commands.requestEnsureVisible();
	caretVisible = true;
	scheduleBlink();
	update();
	Q_EMIT documentEdited();
}

void QtEditorView::repaintAndFollow()
{
	highlight.poll();
	highlight.highlightContent();
	commands.requestEnsureVisible();
	update();
}

void QtEditorView::toggleFindBar()
{
	if (findBar->isVisible())
		findBar->closeBar();
	else
		findBar->open();
}

void QtEditorView::goToLineDialog()
{
	// In-editor overlay (ImGui parity): small floating input, same window.
	if (!lineJumpInput)
	{
		lineJumpInput = new QLineEdit(this);
		lineJumpInput->setPlaceholderText("Go to line…");
		lineJumpInput->setFixedWidth(160);
		lineJumpInput->setAutoFillBackground(true);
		lineJumpInput->installEventFilter(this);
	}
	lineJumpInput->setText(QString::number(
		viewState.selections[viewState.primaryIndex].headRow + 1));
	lineJumpInput->show();
	lineJumpInput->setFocus();
	lineJumpInput->selectAll();
}

bool QtEditorView::eventFilter(QObject *watched, QEvent *event)
{
	if (watched == lineJumpInput && event->type() == QEvent::KeyPress)
	{
		auto *key = static_cast<QKeyEvent *>(event);
		if (key->key() == Qt::Key_Escape)
		{
			lineJumpInput->hide();
			setFocus();
			return true;
		}
		if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter)
		{
			const int line = lineJumpInput->text().toInt();
			lineJumpInput->hide();
			setFocus();
			if (line >= 1 && line <= state.lineCount())
			{
				commands.goToLine(line - 1); // commands API is 0-based
				scrollBar->setValue(std::max(
					0, viewState.selections[viewState.primaryIndex].headRow -
						   visibleLines() / 2));
				scheduleBlink();
				update();
			}
			return true;
		}
	}
	return QWidget::eventFilter(watched, event);
}

void QtEditorView::scheduleBlink()
{
	blinkClock.restart();
	caretVisible = true;
	blinkTimer->start();
}

bool QtEditorView::rainbowMode() const
{
	return appSettings.settings.value("rainbow", true);
}

void QtEditorView::keyPressEvent(QKeyEvent *event)
{
	const bool shift = event->modifiers() & Qt::ShiftModifier;
	const bool ctrl = event->modifiers() & Qt::ControlModifier;
	const bool meta = event->modifiers() & Qt::MetaModifier; // Cmd
	const bool alt = event->modifiers() & Qt::AltModifier;	 // Option
	const bool primary = ctrl || meta;

	// Option/Alt: word left/right, add caret above/below (not with Cmd/Ctrl).
	if (alt && !primary)
	{
		switch (event->key())
		{
		case Qt::Key_Left: commands.moveWordLeft(shift); break;
		case Qt::Key_Right: commands.moveWordRight(shift); break;
		case Qt::Key_Up: commands.addCursorAbove(); break;
		case Qt::Key_Down: commands.addCursorBelow(); break;
		default: QWidget::keyPressEvent(event); return;
		}
		afterEdit();
		return;
	}

	if (primary)
	{
		switch (event->key())
		{
		case Qt::Key_A: commands.selectAll(); break;
		case Qt::Key_Z: commands.undo(); break;
		case Qt::Key_Y: commands.redo(); break;
		case Qt::Key_C: commands.copy(); break;
		case Qt::Key_X: commands.cut(); break;
		case Qt::Key_V: commands.paste(); break;
		case Qt::Key_S: commands.save(); break;
		case Qt::Key_Left: commands.moveLineStart(shift); break;
		case Qt::Key_Right: commands.moveLineEnd(shift); break;
		case Qt::Key_Up: commands.moveLines(-5, shift); break;
		case Qt::Key_Down: commands.moveLines(5, shift); break;
		default: QWidget::keyPressEvent(event); return;
		}
		afterEdit();
		return;
	}

	switch (event->key())
	{
	case Qt::Key_Left: commands.moveLeft(shift); break;
	case Qt::Key_Right: commands.moveRight(shift); break;
	case Qt::Key_Up: commands.moveUp(shift); break;
	case Qt::Key_Down: commands.moveDown(shift); break;
	case Qt::Key_Home: commands.moveLineStart(shift); break;
	case Qt::Key_End: commands.moveLineEnd(shift); break;
	case Qt::Key_Return:
	case Qt::Key_Enter: commands.insertNewline(); break;
	case Qt::Key_Backspace: commands.deleteLeft(false); break;
	case Qt::Key_Delete: commands.deleteRight(false); break;
	case Qt::Key_Tab: commands.indent(); break;
	case Qt::Key_Backtab: commands.outdent(); break;
	default:
	{
		const QString text = event->text();
		if (!text.isEmpty())
		{
			commands.typeText(text.toUtf8().constData());
			break;
		}
		QWidget::keyPressEvent(event);
		return;
	}
	}
	afterEdit();
}

void QtEditorView::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	if (findBar && findBar->isVisible())
		findBar->setGeometry(0, 0, width(), findBar->sizeHint().height());
	if (lineJumpInput && lineJumpInput->isVisible())
		lineJumpInput->move(width() - lineJumpInput->width() - 24, titleBarPx + 6);
	scrollBar->setGeometry(width() - 14, 0, 14, height());
	scrollBar->setPageStep(std::max(1, visibleLines() - 1));
	scrollBar->setRange(0, maxScrollLine());
}

void QtEditorView::wheelEvent(QWheelEvent *event)
{
	// Trackpads report small pixel-ish deltas; mice report 120/notch.
	const int notches = event->angleDelta().y() / 40;
	scrollBar->setValue(scrollBar->value() - notches);
	update();
}

void QtEditorView::mousePressEvent(QMouseEvent *event)
{
	if (event->button() != Qt::LeftButton)
		return;
	const int row = rowAtY(static_cast<int>(event->position().y()));
	const int column = columnAtX(row, static_cast<int>(event->position().x()));
	commands.setCursor(row, column, event->modifiers() & Qt::ShiftModifier);
	dragging = true;
	caretVisible = true;
	scheduleBlink();
	update();
}

void QtEditorView::mouseMoveEvent(QMouseEvent *event)
{
	if (!dragging)
		return;
	const int row = rowAtY(static_cast<int>(event->position().y()));
	const int column = columnAtX(row, static_cast<int>(event->position().x()));
	commands.setCursor(row, column, true);
	update();
}

void QtEditorView::mouseReleaseEvent(QMouseEvent *) { dragging = false; }
