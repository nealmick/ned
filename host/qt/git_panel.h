/*
	File: host/qt/git_panel.h
	Description: Git browser panel for the Qt host's left sidebar (the
	activity bar's Source Control page). Two sections in a splitter: a
	"Changes" section listing staged and unstaged files, and a "Graph"
	section drawing the branch history as the classic git flow-chart
	(lanes, nodes, fork curves). Read-only for now — staging/discard/
	commit actions come later.

	Data runs on a worker thread (git_status_list_new can cost hundreds
	of ms — see git_service.h) and lands on the UI thread via a polled
	handoff, the same pattern EditorGit uses.
*/

#pragma once

#ifndef NED_ENABLE_GIT
#define NED_ENABLE_GIT 1
#endif

#include "../../../util/settings.h"

#include <QWidget>

#if NED_ENABLE_GIT
#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <thread>
#endif

class QLabel;
class QTimer;
class QTreeWidget;
class QTreeWidgetItem;
class NedSplitter;

#if NED_ENABLE_GIT
#include "editor/services/git/git_repo.h"
#endif

class GitPanel : public QWidget
{
	Q_OBJECT

  public:
	explicit GitPanel(const Settings &settings, QWidget *parent = nullptr);
	~GitPanel() override;

	// Repo to browse (empty = no workspace → sections show empty lists).
	void setWorkspaceRoot(const QString &root);

  Q_SIGNALS:
	// Clicking a changed file: open its diff tab (path is repo-relative;
	// staged = HEAD-vs-index, otherwise index-vs-worktree).
	void diffRequested(const QString &repoRelativePath, bool staged);

  protected:
	void paintEvent(QPaintEvent *event) override;
#if NED_ENABLE_GIT
	bool eventFilter(QObject *watched, QEvent *event) override;
	void hideEvent(QHideEvent *event) override;

  private:
	void kickScan();	 // start the worker if idle
	void applyResults(); // UI thread: adopt the last worker payload
	// Graph click → the commit modal (settings-popup pattern: in-window
	// card + scrim; details fetched off-thread and cached per sha).
	void onCommitActivated(const QString &fullId);
	void openCommitModal(const QString &fullId);
	void closeCommitModal();
	void repositionCommitModal();
	struct Details; // {message, files} — see the cache below
	QString buildCommitHtml(const struct Details &d, const QString &fullId);
#else
  private:
#endif

	const Settings *m_settings;

	NedSplitter *m_splitter = nullptr;
	QLabel *m_stagedHeader = nullptr;
	QLabel *m_unstagedHeader = nullptr;
	QTreeWidget *m_stagedList = nullptr;
	QTreeWidget *m_unstagedList = nullptr;
	QTreeWidget *m_graph = nullptr; // rows painted by GitGraph::Delegate

#if NED_ENABLE_GIT
	// Worker side: GitRepo is touched ONLY on the worker thread.
	QString m_root;
	GitRepo m_repo;
	std::thread m_worker;
	std::atomic<bool> m_inFlight{false};
	std::atomic<bool> m_shuttingDown{false};
	std::mutex m_mu; // guards the pending payload below
	struct Payload
	{
		std::vector<std::string> staged;
		std::vector<std::string> unstaged;
		std::vector<GitRepo::Commit> commits;
		bool fresh = false;
	} m_pending;

	// Last applied scan result — applyResults skips no-op refreshes (an
	// identical repopulate just flickers the lists).
	std::vector<std::string> m_lastStaged, m_lastUnstaged;
	size_t m_lastCommitCount = 0;

	// Commit modal details. The fetch runs on its own thread with its OWN
	// GitRepo (never shares m_repo with the scan worker) and posts back
	// via QueuedConnection; results cache per full sha.
	struct Details
	{
		std::string message;
		std::vector<GitRepo::FileStat> files;
	};
	std::map<QString, Details> m_details; // UI-thread cache
	// cpp-local GraphDelegate can't be named here; clear-highlight hook.
	std::function<void(int)> m_setHoverRow;
	QString m_modalRequestId;		  // commit the pending fetch should open
	QString m_remoteUrl;			  // origin URL (worker thread resolves once)
	QWidget *m_commitScrim = nullptr; // settings-style dim + click-dismiss
	QWidget *m_commitCard = nullptr;
	QLabel *m_commitLabel = nullptr;
	GitRepo m_detailRepo; // detail-worker only
	std::thread m_detailWorker;
	std::atomic<bool> m_detailInFlight{false};
#endif
	QTimer *m_tick = nullptr;
};
