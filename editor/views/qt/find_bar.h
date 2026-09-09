/*
	File: views/qt/find_bar.h
	Description: In-document find bar for the Qt backend (Cmd/Ctrl+F).
	Searches the EditorState text and drives the selection through
	EditorCommands::setSelection. Replace via typeText over selection.
*/

#pragma once

#include <QWidget>

class QKeyEvent;
class QLineEdit;
class QPushButton;
class QResizeEvent;
class EditorFrame;

class FindBar : public QWidget
{
	Q_OBJECT

  public:
	explicit FindBar(EditorFrame *editor, QWidget *parent = nullptr);

	void open(); // show, focus input, keep last term
	void dismiss();

  Q_SIGNALS:
	// Wrap state changed — the bar's height changed with it, so the view
	// must re-displace the text area (topInset).
	void heightChanged();

  protected:
	void keyPressEvent(QKeyEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;

  private:
	void find(bool backwards);
	void replaceOne();
	void replaceAll();
	// Rebuilds the widget rows for the current wrap state (responsive:
	// one row when wide, Find row + Replace row when narrow).
	void buildLayout();

	EditorFrame *editor = nullptr;
	QLineEdit *input = nullptr;
	QLineEdit *replaceInput = nullptr;
	QPushButton *prevBtn = nullptr;
	QPushButton *nextBtn = nullptr;
	QPushButton *replaceBtn = nullptr;
	QPushButton *allBtn = nullptr;
	QPushButton *countLabel = nullptr;
	int matchIndex = -1;
	bool wrapped = false;
	int singleRowMin = 0; // width at which the one-row layout stops fitting
};
