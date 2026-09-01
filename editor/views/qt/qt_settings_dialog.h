/*
	File: views/qt/qt_settings_dialog.h
	Description: Settings dialog for the Qt backend — edits the same
	Settings profile the ImGui build uses (theme, font size, feature
	toggles) and persists it via Settings::saveSettings.
*/

#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QSpinBox;

class Settings;

class QtSettingsDialog : public QDialog
{
	Q_OBJECT

  public:
	explicit QtSettingsDialog(Settings &settings, QWidget *parent = nullptr);

  private:
	void save();

	Settings &appSettings;
	QComboBox *themeBox = nullptr;
	QSpinBox *fontSizeBox = nullptr;
	QCheckBox *lineNumbersBox = nullptr;
	QCheckBox *gitGutterBox = nullptr;
	QCheckBox *rainbowBox = nullptr;
	QCheckBox *treeSitterBox = nullptr;
};
