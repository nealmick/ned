/*
	File: host/qt/settings_view.h
	Description: Settings dialog for the Qt backend — mirrors the ImGui
	settings window (host/imgui/settings_view.cpp): profile picker, font,
	font-size slider, background color, macOS opacity/blur and the editor
	toggles. Edits the same Settings profile and persists it via
	Settings::saveSettings. LIVE-APPLY (ImGui parity): every change commits
	immediately — no OK/Cancel, just Close.
*/

#pragma once

#include <QColor>
#include <QDialog>
#include <QString>

class QCheckBox;
class QComboBox;
class QPushButton;
class QSlider;
class QSpinBox;
class QWidget;

class Settings;

class SettingsView : public QDialog
{
	Q_OBJECT

  public:
	explicit SettingsView(Settings &settings, QWidget *parent = nullptr);

  Q_SIGNALS:
	// "Language Servers…" pressed — the host shows the LSP dashboard
	// (parity with the ImGui settings window's dashboard button).
	void lspDashboardRequested();
	// File-explorer / terminal panel checkboxes — applied IMMEDIATELY
	// (ImGui parity: its checkboxes call toggleSidebar/toggleTerminal on
	// click), unlike the staged color edits that only commit on OK.
	void panelTogglesChanged(bool sidebar, bool terminal);
	// Any setting committed (live-apply: every change saves + persists).
	// The Qt host has no frame loop to poll Settings::needsApply, so the
	// dialog tells it directly — the host re-applies fonts/theme/chrome.
	void applied();

  private:
	// ImGui settings parity: changes apply on interaction (no OK).
	void commit();			 // write every control to settings + persist + applied()
	void syncFromSettings(); // re-read controls from settings (profile switch)
	void onProfileChanged();
	// Background color from the profile (theme's own fallback applies).
	QColor readPendingBg() const;
	// Resolve the stored font (possibly a bundled file stem) onto fontBox.
	void selectStoredFont();
	// syncFromSettings with change signals suppressed.
	void resyncFromSettings();
	QWidget *makeSection(const QString &title); // bold header + hairline
	QPushButton *makeColorSwatch(const QColor &color);
	void paintSwatch(QPushButton *swatch, const QColor &color);

	Settings &appSettings;
	std::string stagedProfileFile; // profile the background color was staged from
	QComboBox *themeBox = nullptr;
	QComboBox *fontBox = nullptr;
	QSlider *fontSizeSlider = nullptr;
	QSpinBox *fontSizeBox = nullptr;
	QPushButton *bgButton = nullptr;
	QColor pendingBg;
	QSlider *opacitySlider = nullptr;
	QCheckBox *blurBox = nullptr;
	QCheckBox *minimapBox = nullptr;
	QCheckBox *wordWrapBox = nullptr;
	QCheckBox *treeSitterBox = nullptr;
	QCheckBox *gitGutterBox = nullptr;
	QCheckBox *sidebarBox = nullptr;
	QCheckBox *terminalBox = nullptr;
	bool syncing = false; // programmatic widget updates must not re-commit
};
