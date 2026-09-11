/*
	File: host/qt/activity_bar.h
	Description: VSCode-style activity bar for the Qt host — the vertical
	strip of panel icons to the LEFT of the file tree. Each button switches
	the dock's stacked panel (files tree / git browser / ...). Icons are the
	bundled codicons (resources/icons), rendered direct from SVG at exact
	device pixels and tinted per state — no QIcon round-trip (it rescales
	and softens the glyph).
*/

#pragma once

#include "../../../util/settings.h"

#include <QList>
#include <QWidget>

class NedActivityBar : public QWidget
{
	Q_OBJECT

  public:
	// Page indices match the QStackedWidget the host wires up.
	enum Panel { PanelFiles = 0, PanelGit = 1 };

	explicit NedActivityBar(const Settings &settings, QWidget *parent = nullptr);

	// Selects the active button WITHOUT emitting panelRequested (restores,
	// host-side state changes).
	void setActivePanel(int panel);
	int activePanel() const { return m_active; }

  Q_SIGNALS:
	void panelRequested(int panel);

  protected:
	void paintEvent(QPaintEvent *event) override;

  private:
	class Button;
	Button *makeButton(const QString &iconKey, const QString &tip, int panel);

	const Settings *m_settings;
	class Button;
	QList<Button *> m_buttons; // creation order == panel order
	int m_active = PanelFiles;
};
