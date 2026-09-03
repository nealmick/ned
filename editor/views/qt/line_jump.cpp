#include "line_jump.h"

#include "host/qt/theme.h"

#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

#include <algorithm>

LineJump::LineJump(int currentLine, int lines, QWidget *parent)
	: QDialog(parent, Qt::Popup), lineCount(lines)
{
	setObjectName("nedLineJump");
	// Translucent window so the card's rounded corners show — the same
	// two-layer pattern as FileFinderView (opaque card on a clear popup).
	setAttribute(Qt::WA_TranslucentBackground);
	auto *outer = new QVBoxLayout(this);
	outer->setContentsMargins(0, 0, 0, 0);
	auto *card = new QWidget(this);
	card->setAttribute(Qt::WA_StyledBackground, true);
	// Rule scoped to the card only (selector-less blocks would also draw
	// the border around the title and input — the finder's "extra
	// borders" bug).
	card->setObjectName("nedPopoverCard");
	card->setStyleSheet(QStringLiteral("#nedPopoverCard { background: %1; "
									   "border: 1px solid rgba(128, 128, 128, 110); "
									   "border-radius: 11px; }")
							.arg(NedQtTheme::popoverColor()));
	outer->addWidget(card);

	auto *layout = new QVBoxLayout(card);
	layout->setContentsMargins(12, 10, 12, 10);
	layout->setSpacing(6);

	auto *title = new QLabel("Go to Line", card);
	title->setAlignment(Qt::AlignHCenter);
	title->setStyleSheet("font-weight: 600;");
	layout->addWidget(title);

	input = new QLineEdit(card);
	input->setPlaceholderText("Line number (1–" + QString::number(lineCount) + ")");
	input->setText(QString::number(currentLine));
	layout->addWidget(input);

	input->setFocus();
	input->selectAll();
}

void LineJump::keyPressEvent(QKeyEvent *event)
{
	switch (event->key())
	{
	case Qt::Key_Escape:
		reject();
		return;
	case Qt::Key_Return:
	case Qt::Key_Enter: {
		bool valid = false;
		const int line = input->text().toInt(&valid);
		if (valid)
		{
			// Out-of-range numbers clamp to the ends rather than being
			// rejected — typing past the end lands on the last line.
			Q_EMIT jumpRequested(std::clamp(line, 1, lineCount));
			accept();
		}
		return;
	}
	default:
		break;
	}
	QDialog::keyPressEvent(event);
}
