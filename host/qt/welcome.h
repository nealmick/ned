/*
	File: host/qt/welcome.h
	Description: Pre-workspace welcome surface for the Qt host (parallel
	of host/imgui/welcome.cpp): logo + title + Open Folder, centered.
	Emits openFolderRequested — the host owns the folder dialog.
*/

#pragma once

#include <QWidget>

class WelcomePage : public QWidget
{
	Q_OBJECT

  public:
	explicit WelcomePage(QWidget *parent = nullptr);

  Q_SIGNALS:
	void openFolderRequested();
};
