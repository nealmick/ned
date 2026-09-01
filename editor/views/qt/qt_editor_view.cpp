#include "qt_editor_view.h"

#include "../../util/utf8.h"
#include "../../../util/settings.h"
#include "ned_color_qt.h"
#include "qt_find_bar.h"

#include <QShortcut>

#include <QFontMetrics>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTimer>
#include <QWheelEvent>

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
	connect(blinkTimer, &QTimer::timeout, this, [this] {
		caretVisible = !caretVisible;
		update();
	});
	blinkTimer->start();

	scrollBar = new QScrollBar(Qt::Vertical, this);
	connect(scrollBar, &QScrollBar::valueChanged, this,
			[this](int) { update(); });

	// Services (async tree-sitter, autosave) expect per-frame polling; the
	// Qt backend has no frame loop, so a short timer drives them.
	serviceTimer = new QTimer(this);
	serviceTimer->setInterval(30);
	connect(serviceTimer, &QTimer::timeout, this, [this] {
		highlight.poll();
		git.poll();
		if (highlight.visualGeneration() != lastVisualGen)
		{
			lastVisualGen = highlight.visualGeneration();
			update();
		}
	});
	serviceTimer->start();

	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);

	findBar = new QtFindBar(this, this);
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
	QFont font("Menlo"); // macOS monospace; falls back elsewhere
#ifdef _WIN32
	font.setFamily("Consolas");
#endif
	font.setStyleHint(QFont::Monospace);
	font.setPointSize(13);
	setFont(font);

	const QFontMetrics metrics(font);
	lineHeightPx = metrics.height();
	charWidthPx = metrics.horizontalAdvance(' ');
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
	} else
	{
		state.path = "";
	}

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
	return std::max(1, height() / lineHeightPx);
}

int QtEditorView::maxScrollLine() const
{
	return std::max(0, state.lineCount() - visibleLines() + 1);
}

int QtEditorView::rowAtY(int y) const
{
	int row = scrollBar->value() + y / lineHeightPx;
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
	const int rows = std::min(visibleLines() + 1,
							  state.lineCount() - firstRow);

	// Gutter + current-line highlight.
	const Selection &primary = viewState.selections[viewState.primaryIndex];
	painter.setFont(font());
	for (int i = 0; i < rows; ++i)
	{
		const int row = firstRow + i;
		const int y = i * lineHeightPx;
		if (row == primary.headRow)
			painter.fillRect(0, y, width(), lineHeightPx, QColor(0x2a, 0x2a, 0x2a));

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
			painter.fillRect(gutterWidthPx - 6, i * lineHeightPx, 3, lineHeightPx,
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
		const int y = i * lineHeightPx;
		const LineColorSpans &spans = highlight.spansForLine(row);

		int bytePos = 0;
		QColor ink = defaultInk;
		const auto drawSegment = [&](int nextByte) {
			if (nextByte <= bytePos)
				return;
			const QByteArray seg(text.data() + bytePos, nextByte - bytePos);
			const int x = textLeft + bytePos * charWidthPx;
			painter.setPen(ink);
			painter.drawText(x, y, charWidthPx * seg.size(), lineHeightPx,
							 Qt::AlignVCenter, QString::fromUtf8(seg));
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
			painter.drawRect(textLeft + from * charWidthPx, i * lineHeightPx,
							 (to - from) * charWidthPx, lineHeightPx);
		}
	}

	if (caretVisible)
	{
		painter.setPen(QColor(255, 120, 255));
		for (const Selection &sel : viewState.selections)
		{
			const int i = sel.headRow - firstRow;
			if (i < 0 || i >= rows + 1)
				continue;
			painter.drawLine(textLeft + sel.headColumn * charWidthPx,
							 i * lineHeightPx + 2,
							 textLeft + sel.headColumn * charWidthPx,
							 (i + 1) * lineHeightPx - 2);
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

void QtEditorView::scheduleBlink()
{
	blinkTimer->start();
}

void QtEditorView::keyPressEvent(QKeyEvent *event)
{
	const bool sel = event->modifiers() & Qt::ShiftModifier;
	const bool ctrl = event->modifiers() & Qt::ControlModifier;
	const bool meta = event->modifiers() & Qt::MetaModifier;
	const bool mod = ctrl || meta;

	switch (event->key())
	{
	case Qt::Key_Left: mod ? commands.moveWordLeft(sel) : commands.moveLeft(sel); break;
	case Qt::Key_Right: mod ? commands.moveWordRight(sel) : commands.moveRight(sel); break;
	case Qt::Key_Up: commands.moveUp(sel); break;
	case Qt::Key_Down: commands.moveDown(sel); break;
	case Qt::Key_Home: mod ? commands.moveDocStart(sel) : commands.moveLineStart(sel); break;
	case Qt::Key_End: mod ? commands.moveDocEnd(sel) : commands.moveLineEnd(sel); break;
	case Qt::Key_Return:
	case Qt::Key_Enter: commands.insertNewline(); break;
	case Qt::Key_Backspace: commands.deleteLeft(mod); break;
	case Qt::Key_Delete: commands.deleteRight(mod); break;
	case Qt::Key_Tab: commands.indent(); break;
	case Qt::Key_Backtab: commands.outdent(); break;
	default:
		if (mod)
		{
			switch (event->key())
			{
			case Qt::Key_Z: commands.undo(); break;
			case Qt::Key_Y: commands.redo(); break;
			case Qt::Key_A: commands.selectAll(); break;
			case Qt::Key_C: commands.copy(); break;
			case Qt::Key_X: commands.cut(); break;
			case Qt::Key_V: commands.paste(); break;
			case Qt::Key_S: commands.save(); break;
			default: QWidget::keyPressEvent(event); return;
			}
			afterEdit();
			return;
		}
		const QString text = event->text();
		if (!text.isEmpty())
			commands.typeText(text.toUtf8().constData());
		break;
	}
	afterEdit();
}

void QtEditorView::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	if (findBar)
		findBar->setGeometry(0, 0, width(), findBar->sizeHint().height());
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
