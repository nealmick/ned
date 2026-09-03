/*
	File: files/views/qt/qt_file_finder.h
	Description: Ctrl+P project file finder for the Qt backend (parallel
	of files/views/imgui/file_finder_view) — popup with fuzzy filtering
	over a background scan of the workspace. Scan rules and matching come
	from the shared backend-neutral model (files/file_finder_match.h).
*/

#pragma once

#include <QDialog>
#include <QStringList>

#include "../../file_finder_match.h"

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
