/*
	File: host/qt/qt_host.h
	Description: Qt application shell — native QMainWindow host for the ned
	core. Owns window lifecycle, tabs, sidebar, finder and the NSWindow
	chrome on macOS; editor surface lives in editor/views/qt/.
*/

#pragma once

#include "util/settings.h"

#include <QMainWindow>

class QTabWidget;
class QtEditorView;
class QtFileSidebar;

class NedQtHost : public QMainWindow
{
	Q_OBJECT

  public:
	explicit NedQtHost(QWidget *parent = nullptr);
	~NedQtHost() override;

  private:
	void applyNativeChrome();

	void openPath(const QString &path, bool focus);
	void openWorkspace(const QString &root);
	void showWelcome();
	void showEvent(QShowEvent *event) override;
	void refreshTabTitle(int index);
	void applyFontToEditors();

	Q_SIGNALS:
	void sidebarToggleRequested();
	void settingsRequested();

  private:
	Settings settings;
	QTabWidget *tabs = nullptr;
	QtFileSidebar *sidebar = nullptr;
	QString workspaceRoot;
	bool chromeApplied = false;
	int untitledCounter = 1;
};
