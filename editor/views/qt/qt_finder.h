/*
	File: views/qt/qt_finder.h
	Description: Ctrl+P project file finder for the Qt backend — popup with
	fuzzy filtering over a background scan of the workspace. Model logic
	(scan) mirrors files/file_finder.cpp; kept Qt-side until a shared
	backend-neutral matcher is extracted.
*/

#pragma once

#include <QDialog>
#include <QStringList>

#include <atomic>
#include <thread>

class QLineEdit;
class QListWidget;

class QtFileFinder : public QDialog
{
	Q_OBJECT

  public:
	explicit QtFileFinder(const QString &workspace, QWidget *parent = nullptr);
	~QtFileFinder() override;

  Q_SIGNALS:
	void fileSelected(const QString &path);

  protected:
	void keyPressEvent(QKeyEvent *event) override;
	bool eventFilter(QObject *watched, QEvent *event) override;

  private:
	void restartScan();
	void refilter();

	QString workspace;
	std::atomic<bool> stopScan{false};
	std::thread scanThread;
	QStringList allFiles; // guarded by scan completion flag
	std::atomic<bool> scanDone{false};

	QLineEdit *input = nullptr;
	QListWidget *results = nullptr;
};

// Simple subsequence fuzzy score: matched-in-order wins, tighter + earlier
// matches score higher. Returns -1 when query is not a subsequence.
int fuzzyMatchScore(const QString &candidate, const QString &query);
