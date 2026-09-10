/*
	File: host/qt/windows_titlebar.h
	Description: Hand-drawn Windows caption strip for the Qt host — a 1:1
	port of the ImGui host's Workbench::renderWindowsTitlebar (host/imgui/
	workbench.cpp): title left, explorer/terminal/settings-gear actions,
	then the native-integrated min/max/close cluster. The window frame
	itself is stripped by host/qt/windows_chrome.cpp (same WM_NCCALCSIZE /
	WM_NCHITTEST approach as util/windows_window.cpp), so dragging the bar,
	double-click maximize, Aero snap, and the Win11 snap-layout flyout all
	keep working through the OS.
*/

#pragma once

// NOT #ifdef _WIN32-guarded: moc must see the Q_OBJECT class to generate
// its metaobject, and moc does not define _WIN32 — a guarded header moc's
// to nothing and the signals/vtable symbols vanish at link time (the CI
// failure this lesson comes from). Only the IMPLEMENTATION is Windows:
// windows_titlebar.cpp compiles #ifdef _WIN32 and CMake adds it to ned_qt
// on WIN32 only, so on other platforms this declaration is simply never
// included by anything.

#include "../../../util/settings.h"

#include <QList>
#include <QWidget>

class QPaintEvent;
class QMouseEvent;
class QEvent;

class NedQtTitleBar : public QWidget
{
	Q_OBJECT

  public:
	explicit NedQtTitleBar(const Settings &settings, QWidget *parent = nullptr);
	~NedQtTitleBar() override;

	// Re-derive the bar height from the current app font (initial + after
	// a profile font change).
	void syncMetrics();

  Q_SIGNALS:
	void sidebarToggleRequested();
	void terminalToggleRequested();
	void settingsRequested();

  protected:
	void paintEvent(QPaintEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void leaveEvent(QEvent *event) override;

  private:
	// One button rect (logical px) + identity. Roles: 0 sidebar, 1 terminal,
	// 2 gear, 3 min, 4 max, 5 close — terminal is compiled out where the Qt
	// host has no terminal panel, so indices must not be positional.
	enum Role {
		RoleSidebar = 0,
		RoleTerminal = 1,
		RoleGear = 2,
		RoleMin = 3,
		RoleMax = 4,
		RoleClose = 5
	};
	struct Btn
	{
		QRectF rect;
		int role;
		int ht; // HTCLIENT = Qt-handled action; HT* = native caption button
	};
	QList<Btn> layoutButtons() const;

	// wndproc → repaint (caption hover / maximize state changed).
	static void chromeNotifyStatic(void *ctx);
	void refreshChrome();

	const Settings *m_settings;
	int m_hoverAction = -1; // hovered action button's Role (-1 = none)
};
