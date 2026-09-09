/*
	File: files/views/qt/file_finder_view.h
	Description: Ctrl+P project file finder for the Qt backend (parallel
	of files/views/imgui/file_finder_view) — popup with fuzzy filtering
	over a background scan of the workspace. Scan rules and matching come
	from the shared backend-neutral model (files/file_finder_match.h).
*/

#pragma once

#include <QDialog>

#include "../../file_finder.h"
#include "../../file_finder_match.h"

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

class QLineEdit;
class QListWidget;

class FileFinderView : public QDialog
{
	Q_OBJECT

  public:
	explicit FileFinderView(const QString &workspace, QWidget *parent = nullptr);
	~FileFinderView() override;

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
	// Core scan results (scanWorkspaceFiles) — handed off via shared_ptr so
	// early dialog destruction can't dangle the worker's output.
	std::shared_ptr<std::vector<FileEntry>> allFiles;
	std::atomic<bool> scanDone{false};

	QLineEdit *input = nullptr;
	QListWidget *results = nullptr;
};
