#include "qt_find_bar.h"

#include "qt_editor_view.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include <algorithm>

QtFindBar::QtFindBar(QtEditorView *view, QWidget *parent)
	: QWidget(parent), editor(view)
{
	auto *layout = new QHBoxLayout(this);
	layout->setContentsMargins(6, 2, 6, 2);

	input = new QLineEdit(this);
	input->setPlaceholderText("Find");
	input->setFixedWidth(220);
	layout->addWidget(input);

	replaceInput = new QLineEdit(this);
	replaceInput->setPlaceholderText("Replace");
	replaceInput->setFixedWidth(180);
	layout->addWidget(replaceInput);

	auto *prev = new QPushButton("↑", this);
	auto *next = new QPushButton("↓", this);
	auto *replaceBtn = new QPushButton("Replace", this);
	auto *allBtn = new QPushButton("All", this);
	countLabel = new QPushButton("0 matches", this);
	countLabel->setFlat(true);
	countLabel->setEnabled(false);
	for (QWidget *w : {static_cast<QWidget *>(prev), static_cast<QWidget *>(next),
					   static_cast<QWidget *>(replaceBtn), static_cast<QWidget *>(allBtn),
					   static_cast<QWidget *>(countLabel)})
		layout->addWidget(w);
	layout->addStretch();

	connect(next, &QPushButton::clicked, this, [this] { find(false); });
	connect(prev, &QPushButton::clicked, this, [this] { find(true); });
	connect(replaceBtn, &QPushButton::clicked, this, &QtFindBar::replaceOne);
	connect(allBtn, &QPushButton::clicked, this, &QtFindBar::replaceAll);
	connect(input, &QLineEdit::returnPressed, this, [this] { find(false); });
	connect(input, &QLineEdit::textChanged, this, [this] {
		matchIndex = -1;
		find(false);
	});

	hide();
}

void QtFindBar::open()
{
	show();
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
			matches.push_back({r, static_cast<int>(hit),
							   static_cast<int>(needle.size())});
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

	const Selection &caret = editor->viewport().selections[editor->viewport().primaryIndex];
	if (backwards)
	{
		// Last match strictly before the caret.
		auto it = std::find_if(matches.rbegin(), matches.rend(), [&](const Match &m) {
			return m.row < caret.headRow ||
				   (m.row == caret.headRow && m.col < caret.headColumn);
		});
		if (it == matches.rend())
			it = matches.rbegin();
		matchIndex = static_cast<int>(matches.rend() - it) - 1;
	} else
	{
		auto it = std::find_if(matches.begin(), matches.end(), [&](const Match &m) {
			return m.row > caret.headRow ||
				   (m.row == caret.headRow &&
					m.col + m.len > caret.headColumn);
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
			matches.push_back({r, static_cast<int>(hit),
							   static_cast<int>(needle.size())});
			pos = hit + 1;
		}
	}

	for (auto it = matches.rbegin(); it != matches.rend(); ++it)
	{
		editor->commandHandler().setSelection(it->row, it->col, it->row, it->col + it->len);
		editor->commandHandler().typeText(replacement);
	}
	editor->repaintAndFollow();
	countLabel->setText(QString("replaced %1").arg(matches.size()));
}

void QtFindBar::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Escape)
	{
		closeBar();
		return;
	}
	QWidget::keyPressEvent(event);
}
