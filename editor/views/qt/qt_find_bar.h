/*
	File: views/qt/qt_find_bar.h
	Description: In-document find bar for the Qt backend (Cmd/Ctrl+F).
	Searches the EditorState text and drives the selection through
	EditorCommands::setSelection. Replace via typeText over selection.
*/

#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QtEditorView;

class QtFindBar : public QWidget
{
	Q_OBJECT

  public:
	explicit QtFindBar(QtEditorView *editor, QWidget *parent = nullptr);

	void open(); // show, focus input, keep last term
	void closeBar();

  protected:
	void keyPressEvent(QKeyEvent *event) override;

  private:
	void find(bool backwards);
	void replaceOne();
	void replaceAll();

	QtEditorView *editor = nullptr;
	QLineEdit *input = nullptr;
	QLineEdit *replaceInput = nullptr;
	QPushButton *countLabel = nullptr;
	int matchIndex = -1;
};
