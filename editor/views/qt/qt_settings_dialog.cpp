#include "qt_settings_dialog.h"

#include "../../../util/settings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QVBoxLayout>

QtSettingsDialog::QtSettingsDialog(Settings &settings, QWidget *parent)
	: QDialog(parent), appSettings(settings)
{
	setWindowTitle("Settings");
	setModal(true);

	auto *layout = new QVBoxLayout(this);
	auto *form = new QFormLayout();

	themeBox = new QComboBox(this);
	if (appSettings.settings.contains("themes") &&
		appSettings.settings["themes"].is_object())
	{
		for (auto it = appSettings.settings["themes"].begin();
			 it != appSettings.settings["themes"].end(); ++it)
			themeBox->addItem(QString::fromStdString(it.key()));
	}
	themeBox->setCurrentText(QString::fromStdString(
		appSettings.settings.value("theme", std::string("default"))));
	form->addRow("Theme", themeBox);

	fontSizeBox = new QSpinBox(this);
	fontSizeBox->setRange(8, 40);
	fontSizeBox->setValue(appSettings.settings.value("fontSize", 13));
	form->addRow("Font size", fontSizeBox);

	lineNumbersBox = new QCheckBox("Line numbers", this);
	form->addRow("", lineNumbersBox);

	gitGutterBox = new QCheckBox("Git changed-line markers", this);
	gitGutterBox->setChecked(
		appSettings.settings.value("git_changed_lines", true));
	form->addRow("", gitGutterBox);

	layout->addLayout(form);

	auto *buttons = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this,
			[this] { save(); accept(); });
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	layout->addWidget(buttons);
}

void QtSettingsDialog::save()
{
	appSettings.settings["theme"] = themeBox->currentText().toStdString();
	appSettings.settings["fontSize"] = fontSizeBox->value();
	appSettings.settings["git_changed_lines"] = gitGutterBox->isChecked();
	appSettings.saveSettings();
	// Views read the profile live (colors via HighlightService, font via host).
	appSettings.requestApply();
}
