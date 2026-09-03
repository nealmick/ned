/*
	File: host/qt/qt_terminal_panel.h
	Description: Qt counterpart of host/imgui/ned_terminal — a fixed bottom
	panel of shell sessions in a non-dockable tab group with a trailing "+"
	to spawn another. Sessions are QTermWidget instances (lib/qtermwidget,
	LXQt). Compiled in the qtermwidget6 target because its headers use the
	`signals:`/`slots:` keywords, which ned_qt disables (QT_NO_KEYWORDS).
*/

#pragma once

#include <QColor>
#include <QFont>
#include <QWidget>

#include <memory>

class QtTerminalPanel : public QWidget
{
  public:
	explicit QtTerminalPanel(QWidget *parent = nullptr);
	~QtTerminalPanel() override;

	// Working directory for the next shell spawn (project root).
	void setProjectRoot(const QString &root);

	bool visible() const;
	void setVisible(bool on) override;
	void toggle();

	// Theme color for the terminal scheme; its ALPHA drives the panel's
	// translucency so the terminal frosts like the rest of the window
	// (the single window tint shows behind the tab strip).
	void setThemeBackground(const QColor &color);

	// Monospace font for live shells (editor profile font parity). The
	// font must be fixed-pitch — qtermwidget derives its cell width from
	// the font, so a proportional family draws glyphs over each other.
	void applyFont(const QFont &font);

	// Test hooks / internal.
	void addSession();
	int sessionCount() const;

  private:
	struct Impl;
	std::unique_ptr<Impl> impl_;
};
