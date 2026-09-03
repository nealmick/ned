/*
	File: views/qt/qt_line_jump.h
	Description: Go-to-line popup for the Qt backend (Ctrl+;). Rendered
	like the file finder: a centered translucent popup whose opaque card
	child carries the popover styling (QSS backgrounds never render on
	the translucent top-level itself).
*/

#pragma once

#include <QDialog>

class QLineEdit;

class QtLineJumpDialog : public QDialog
{
	Q_OBJECT

  public:
	// currentLine pre-fills the input (1-based, editor caret); lineCount
	// bounds what Enter accepts.
	explicit QtLineJumpDialog(int currentLine, int lineCount, QWidget *parent = nullptr);

  Q_SIGNALS:
	void jumpRequested(int line); // 1-based document line

  protected:
	void keyPressEvent(QKeyEvent *event) override;

  private:
	QLineEdit *input = nullptr;
	int lineCount = 0;
};
