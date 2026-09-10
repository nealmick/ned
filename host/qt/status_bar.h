/*
	File: host/qt/status_bar.h
	Description: VSCode-style bottom status bar for the Qt host — git
	branch bottom-left; Ln/Col, indent, line ending and language name on
	the right. Purely a reader: it polls the active editor and the repo's
	HEAD file on timers (no new signals through Workbench/EditorFrame),
	so it can't disturb the editor surface.
*/

#pragma once

#include "../../../util/settings.h"

#include <QWidget>

class QLabel;
class QTimer;
class Workbench;

class NedStatusBar : public QWidget
{
	Q_OBJECT

  public:
	// workbench: source of the active editor (may be null in tests).
	explicit NedStatusBar(Workbench *workbench,
						  const Settings &settings,
						  QWidget *parent = nullptr);

	// Branch segment follows the open workspace (empty = no repo).
	void setWorkspaceRoot(const QString &root);
	// Re-apply the scaled bar font (initial + profile font changes).
	void syncFont();

  protected:
	void paintEvent(QPaintEvent *event) override;

  private:
	void refresh();		  // editor segments (fast tick)
	void refreshBranch(); // git HEAD read (slow tick)
	QString readBranchName() const;

	Workbench *m_workbench;
	const Settings *m_settings;
	QString m_root;

	QWidget *m_branchBox = nullptr;
	QLabel *m_branch = nullptr;
	QLabel *m_ln = nullptr;
	QLabel *m_col = nullptr;
	QLabel *m_indent = nullptr;
	QLabel *m_eol = nullptr;
	QLabel *m_lang = nullptr;

	QString m_branchFull; // branch name (also gates the painted icon)
	QTimer *m_fastTick = nullptr;
	QTimer *m_gitTick = nullptr;
};
