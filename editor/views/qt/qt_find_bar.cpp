#include "qt_find_bar.h"

#include "qt_editor_view.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

QtFindBar::QtFindBar(QtEditorView *view, QWidget *parent) : QWidget(parent), editor(view)
{
	input = new QLineEdit(this);
	input->setPlaceholderText("Find");
	input->setMinimumWidth(80);
	input->setMaximumWidth(280);

	replaceInput = new QLineEdit(this);
	replaceInput->setPlaceholderText("Replace");
	replaceInput->setMinimumWidth(60);
	replaceInput->setMaximumWidth(220);

	prevBtn = new QPushButton("↑", this);
	nextBtn = new QPushButton("↓", this);
	replaceBtn = new QPushButton("Replace", this);
	allBtn = new QPushButton("All", this);
	countLabel = new QPushButton("0 matches", this);
	countLabel->setFlat(true);
	countLabel->setEnabled(false);
	buildLayout();

	connect(nextBtn, &QPushButton::clicked, this, [this] { find(false); });
	connect(prevBtn, &QPushButton::clicked, this, [this] { find(true); });
	connect(replaceBtn, &QPushButton::clicked, this, &QtFindBar::replaceOne);
	connect(allBtn, &QPushButton::clicked, this, &QtFindBar::replaceAll);
	// Return routing lives in keyPressEvent (returnPressed can't see the
	// Shift modifier for previous-match).
	connect(input, &QLineEdit::textChanged, this, [this] {
		matchIndex = -1;
		find(false);
	});

	// No fill of its own: the bar floats on the window's single tint
	// layer like the rest of the editor chrome.
	hide();
}

void QtFindBar::open()
{
	// Re-derive the wrap state NOW: singleRowMin was measured at
	// construction (before the stylesheet/profile fonts landed) and the
	// bar may have been resized while hidden during dock/snap churn.
	if (!wrapped && layout())
		singleRowMin = std::max(singleRowMin, layout()->minimumSize().width());
	const bool wantWrap = width() < singleRowMin + 8;
	bool rewrapped = false;
	if (wantWrap != wrapped)
	{
		wrapped = wantWrap;
		buildLayout();
		rewrapped = true;
	}
	show();
	// After show: the view's heightChanged handler ignores hidden bars.
	if (rewrapped)
		Q_EMIT heightChanged();
	input->setFocus();
	input->selectAll();
	if (!input->text().isEmpty())
	{
		matchIndex = -1;
		find(false);
	}
}

void QtFindBar::closeBar()
{
	hide();
	editor->setFocus();
}

void QtFindBar::find(bool backwards)
{
	const std::string needle = input->text().toStdString();
	const std::string haystack = editor->document().join();
	if (needle.empty())
	{
		countLabel->setText("0 matches");
		return;
	}

	// Collect match positions (row/col) around the caret, then pick the
	// next (or previous) one. Full scan keeps ordering trivially correct.
	struct Match
	{
		int row, col, len;
	};
	std::vector<Match> matches;
	int row = 0, col = 0;
	for (int r = 0; r < editor->document().lineCount(); ++r)
	{
		const std::string line = editor->document().line(r);
		for (size_t pos = 0;;)
		{
			const size_t hit = line.find(needle, pos);
			if (hit == std::string::npos)
				break;
			matches.push_back({r, static_cast<int>(hit), static_cast<int>(needle.size())});
			pos = hit + 1;
		}
	}
	(void)row;
	(void)col;

	if (matches.empty())
	{
		countLabel->setText("0 matches");
		return;
	}

	const Selection &caret =
		editor->viewport().selections[editor->viewport().primaryIndex];
	if (backwards)
	{
		// Last match strictly before the caret.
		auto it = std::find_if(matches.rbegin(), matches.rend(), [&](const Match &m) {
			// The caret sits at the match END after setSelection, so a
			// start-only comparison re-finds the current match; require
			// the match to END strictly before the caret instead.
			return m.row < caret.headRow ||
				   (m.row == caret.headRow && m.col + m.len < caret.headColumn);
		});
		if (it == matches.rend())
			it = matches.rbegin();
		matchIndex = static_cast<int>(matches.rend() - it) - 1;
	} else
	{
		auto it = std::find_if(matches.begin(), matches.end(), [&](const Match &m) {
			return m.row > caret.headRow ||
				   (m.row == caret.headRow && m.col + m.len > caret.headColumn);
		});
		if (it == matches.end())
			it = matches.begin();
		matchIndex = static_cast<int>(it - matches.begin());
	}

	const Match &m = matches[static_cast<size_t>(matchIndex)];
	editor->commandHandler().setSelection(m.row, m.col, m.row, m.col + m.len);
	editor->repaintAndFollow();
	countLabel->setText(QString("%1/%2").arg(matchIndex + 1).arg(matches.size()));
}

void QtFindBar::replaceOne()
{
	if (input->text().isEmpty())
		return;
	if (matchIndex < 0)
		find(false);
	editor->commandHandler().typeText(replaceInput->text().toUtf8().constData());
	editor->repaintAndFollow();
	find(false);
}

void QtFindBar::replaceAll()
{
	if (input->text().isEmpty())
		return;
	const std::string needle = input->text().toStdString();
	const std::string replacement = replaceInput->text().toStdString();

	// Collect matches once (doc coords), then replace back-to-front so
	// earlier offsets stay valid.
	struct Match
	{
		int row, col, len;
	};
	std::vector<Match> matches;
	for (int r = 0; r < editor->document().lineCount(); ++r)
	{
		const std::string line = editor->document().line(r);
		for (size_t pos = 0;;)
		{
			const size_t hit = line.find(needle, pos);
			if (hit == std::string::npos)
				break;
			matches.push_back({r, static_cast<int>(hit), static_cast<int>(needle.size())});
			pos = hit + 1;
		}
	}

	for (auto it = matches.rbegin(); it != matches.rend(); ++it)
	{
		editor->commandHandler().setSelection(
			it->row, it->col, it->row, it->col + it->len);
		editor->commandHandler().typeText(replacement);
	}
	editor->repaintAndFollow();
	countLabel->setText(QString("replaced %1").arg(matches.size()));
}

void QtFindBar::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Escape)
	{
		// Route through the view so the displaced text area is restored.
		editor->closeFindBar();
		return;
	}
	if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
	{
		// QLineEdit emits returnPressed but leaves the event ignored, so
		// an unhandled Return propagates up to the editor and types a
		// newline into the document. Enter (or the ↓ button) finds the
		// next match, Shift+Enter the previous; in the replace field,
		// Enter replaces the current match.
		if (replaceInput->hasFocus())
			replaceOne();
		else
			find(event->modifiers() & Qt::ShiftModifier);
		return;
	}
	QWidget::keyPressEvent(event);
}

void QtFindBar::buildLayout()
{
	// The widgets survive; only the layout is rebuilt (wrapping is rare).
	if (layout())
		delete layout();
	auto *outer = new QVBoxLayout(this);
	outer->setContentsMargins(6, 2, 6, 2);
	outer->setSpacing(2);
	auto *row1 = new QHBoxLayout();
	outer->addLayout(row1);

	if (!wrapped)
	{
		// Wide: one row — Find | Replace | ↑ ↓ Replace All | n/m.
		row1->addWidget(input, 1); // flexible: absorbs leftover width
		row1->addWidget(replaceInput, 1);
		for (QWidget *w : {static_cast<QWidget *>(prevBtn),
						   static_cast<QWidget *>(nextBtn),
						   static_cast<QWidget *>(replaceBtn),
						   static_cast<QWidget *>(allBtn),
						   static_cast<QWidget *>(countLabel)})
			row1->addWidget(w);
		row1->addStretch();
		singleRowMin = outer->minimumSize().width();
		return;
	}

	// Narrow: two rows — Find | ↑ ↓ on top, Replace | Replace All | n/m
	// below (web-style wrap instead of squishing everything into one row).
	row1->addWidget(input, 1);
	row1->addWidget(prevBtn);
	row1->addWidget(nextBtn);
	auto *row2 = new QHBoxLayout();
	row2->addWidget(replaceInput, 1);
	row2->addWidget(replaceBtn);
	row2->addWidget(allBtn);
	row2->addWidget(countLabel);
	outer->addLayout(row2);
}

void QtFindBar::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	// Track the one-row minimum as fonts/styles settle after startup.
	if (!wrapped && layout())
		singleRowMin = std::max(singleRowMin, layout()->minimumSize().width());
	// Wrap when the one-row layout no longer fits; unwrap only with a
	// margin so the mode doesn't flicker at the boundary.
	if (!wrapped && singleRowMin > 0 && width() < singleRowMin + 8)
	{
		wrapped = true;
		buildLayout();
		Q_EMIT heightChanged();
	} else if (wrapped && width() > singleRowMin + 40)
	{
		wrapped = false;
		buildLayout();
		Q_EMIT heightChanged();
	}
}
