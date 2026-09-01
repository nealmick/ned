#include "qt_settings_dialog.h"

#include "../../../util/settings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QDir>
#include <QFontDatabase>
#include <QSpinBox>

#include <filesystem>
#include <QVBoxLayout>

QtSettingsDialog::QtSettingsDialog(Settings &settings, QWidget *parent)
	: QDialog(parent), appSettings(settings)
{
	setWindowTitle("Settings");
	setModal(true);

	auto *layout = new QVBoxLayout(this);
	auto *form = new QFormLayout();

	// Profiles are files in the user config dir (same list as the ImGui
	// settings window); switching reloads the whole profile.
	themeBox = new QComboBox(this);
	namespace fs = std::filesystem;
	const fs::path configDir = Settings::getUserConfigDir();
	std::error_code ec;
	if (fs::is_directory(configDir, ec))
	{
		for (const auto &entry : fs::directory_iterator(configDir, ec))
		{
			const std::string name = entry.path().filename().string();
			if (name.ends_with(".json") && name != "keybinds.json" &&
				name != "lsp.json" && name != "ned.json")
				themeBox->addItem(QString::fromStdString(
					name.substr(0, name.size() - 5)));
		}
	}
	themeBox->addItem("default");
	themeBox->setCurrentText(QString::fromStdString(appSettings.activeProfile()));
	form->addRow("Profile", themeBox);

	// Bundled fonts (resources/fonts) registered into QFontDatabase.
	fontBox = new QComboBox(this);
	const QString fontsDir = QString::fromStdString(
		Settings::getAppResourcesPath()) + "/resources/fonts";
	QDir dir(fontsDir);
	const QStringList fonts =
		dir.entryList({"*.ttf", "*.otf"}, QDir::Files, QDir::Name);
	QStringList families;
	for (const QString &file : fonts)
	{
		const int id = QFontDatabase::addApplicationFont(dir.filePath(file));
		if (id >= 0)
		{
			const QStringList fam =
				QFontDatabase::applicationFontFamilies(id);
			if (!fam.isEmpty())
				families << fam.first();
		}
	}
	families.removeDuplicates();
	fontBox->addItem("System Default");
	fontBox->addItems(families);
	fontBox->setCurrentText(QString::fromStdString(
		appSettings.settings.value("font", std::string("System Default"))));
	form->addRow("Font", fontBox);

	fontSizeBox = new QSpinBox(this);
	fontSizeBox->setRange(8, 40);
	fontSizeBox->setValue(appSettings.settings.value("fontSize", 13));
	form->addRow("Font size", fontSizeBox);

	minimapBox = new QCheckBox("Minimap (coming soon)", this);
	minimapBox->setChecked(appSettings.settings.value("minimap", true));
	minimapBox->setEnabled(false); // not yet ported
	form->addRow("", minimapBox);

	lineNumbersBox = new QCheckBox("Line numbers", this);
	form->addRow("", lineNumbersBox);

	gitGutterBox = new QCheckBox("Git changed-line markers", this);
	gitGutterBox->setChecked(
		appSettings.settings.value("git_changed_lines", true));
	form->addRow("", gitGutterBox);

	rainbowBox = new QCheckBox("Rainbow cursor (steady) vs blinking", this);
	rainbowBox->setChecked(appSettings.settings.value("rainbow", true));
	form->addRow("", rainbowBox);

	treeSitterBox = new QCheckBox("Tree-sitter syntax highlighting", this);
	treeSitterBox->setChecked(appSettings.settings.value("treesitter", true));
	form->addRow("", treeSitterBox);

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
	if (themeBox->currentText() != QString::fromStdString(appSettings.activeProfile()))
		appSettings.switchToProfile(themeBox->currentText().toStdString());
	appSettings.settings["font"] = fontBox->currentText().toStdString();
	appSettings.settings["fontSize"] = fontSizeBox->value();
	appSettings.settings["minimap"] = minimapBox->isChecked();
	appSettings.settings["git_changed_lines"] = gitGutterBox->isChecked();
	appSettings.settings["rainbow"] = rainbowBox->isChecked();
	appSettings.settings["treesitter"] = treeSitterBox->isChecked();
	appSettings.saveSettings();
	// Views read the profile live (colors via HighlightService, font via host).
	appSettings.requestApply();
}
