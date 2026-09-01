/*
	File: host/qt/qt_host.h
	Description: Qt application shell — native QMainWindow host for the ned
	core. Owns window lifecycle and the NSWindow chrome on macOS; editor
	surface views (editor/views/qt/) attach to the central widget area.
*/

#pragma once

#include <memory>

#include <QMainWindow>

class QTabWidget;

class NedQtHost : public QMainWindow
{
	Q_OBJECT

  public:
	explicit NedQtHost(QWidget *parent = nullptr);
	~NedQtHost() override;

  private:
	void applyNativeChrome();

	QTabWidget *tabs = nullptr;
};
