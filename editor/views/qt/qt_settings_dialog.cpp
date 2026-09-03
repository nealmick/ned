#include "qt_settings_dialog.h"

#include "../../../util/settings.h"
#include "qt_fonts.h"
#include "qt_theme.h"

#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFontDatabase>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

#include <algorithm>

#include <filesystem>

QtSettingsDialog::QtSettingsDialog(Settings &settings, QWidget *parent)
	: QDialog(parent), appSettings(settings)
{
	setWindowTitle("Settings");
	setModal(true);
	setMinimumWidth(560);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(22, 18, 22, 18);
	layout->setSpacing(10);

	auto *form = new QFormLayout();
	form->setSpacing(8);

	// Profiles are files in the user config dir — same list and same full
	// filenames ("melange.json") as the ImGui settings window's combo.
	themeBox = new QComboBox(this);
	themeBox->setObjectName(QStringLiteral("profileBox")); // host/check-driver lookup
	for (const std::string &name : appSettings.listProfiles())
		themeBox->addItem(QString::fromStdString(name));
	if (themeBox->findText(QString::fromStdString(appSettings.activeProfileFile())) < 0)
		themeBox->insertItem(0, QString::fromStdString(appSettings.activeProfileFile()));
	themeBox->setCurrentText(QString::fromStdString(appSettings.activeProfileFile()));
	stagedProfileFile = appSettings.activeProfileFile();
	form->addRow("Profile", themeBox);

	// Bundled fonts (resources/fonts) registered into QFontDatabase.
	fontBox = new QComboBox(this);
	const QString fontsDir =
		QString::fromStdString(Settings::getAppResourcesPath()) + "/resources/fonts";
	QDir dir(fontsDir);
	const QStringList fonts = dir.entryList({"*.ttf", "*.otf"}, QDir::Files, QDir::Name);
	QStringList families;
	for (const QString &file : fonts)
	{
		const int id = QFontDatabase::addApplicationFont(dir.filePath(file));
		if (id >= 0)
		{
			NedQtFonts::registerFontFile(dir.filePath(file), id);
			const QStringList fam = QFontDatabase::applicationFontFamilies(id);
			if (!fam.isEmpty())
				families << fam.first();
		}
	}
	families.removeDuplicates();
	fontBox->addItem("System Default");
	fontBox->addItems(families);
	{
		// The profile may store a bundled file stem ("SourceCodePro-Regular",
		// the ImGui default) — resolve it to the real family for display.
		const QString stored = QString::fromStdString(
			appSettings.settings.value("font", std::string("System Default")));
		QString family = NedQtFonts::resolveFamily(stored);
		if (family.isEmpty())
			family = stored;
		fontBox->setCurrentText(fontBox->findText(family) >= 0 ? family
															   : "System Default");
	}
	form->addRow("Font", fontBox);

	// Font size: slider like the ImGui window (4–64), spinbox as readout.
	fontSizeSlider = new QSlider(Qt::Horizontal, this);
	fontSizeSlider->setRange(4, 64);
	fontSizeBox = new QSpinBox(this);
	fontSizeBox->setRange(4, 64);
	const int initialSize = static_cast<int>(appSettings.settings.value("fontSize", 13));
	fontSizeSlider->setValue(initialSize);
	fontSizeBox->setValue(initialSize);
	connect(fontSizeSlider, &QSlider::valueChanged, this, [this](int value) {
		fontSizeBox->setValue(value);
	});
	connect(fontSizeBox, &QSpinBox::valueChanged, this, [this](int value) {
		fontSizeSlider->setValue(value);
	});
	auto *sizeRow = new QWidget(this);
	auto *sizeLayout = new QHBoxLayout(sizeRow);
	sizeLayout->setContentsMargins(0, 0, 0, 0);
	sizeLayout->setSpacing(10);
	sizeLayout->addWidget(fontSizeSlider, 1);
	sizeLayout->addWidget(fontSizeBox);
	form->addRow("Font size", sizeRow);

	// Background color: swatch opens a picker; applies on pick (live).
	// Same fallback the theme uses, so a profile without the key swatches
	// what the app actually renders.
	pendingBg = NedQtTheme::colorFromJson(appSettings.settings.contains("backgroundColor")
											  ? &appSettings.settings["backgroundColor"]
											  : nullptr,
										  NedQtTheme::defaultBackground());
	bgButton = makeColorSwatch(pendingBg);
	connect(bgButton, &QPushButton::clicked, this, [this] {
		const QColor picked = QColorDialog::getColor(
			pendingBg, this, "Background Color", QColorDialog::ShowAlphaChannel);
		if (picked.isValid())
		{
			pendingBg = picked;
			paintSwatch(bgButton, picked);
			commit();
		}
	});
	form->addRow("Background color", bgButton);

	layout->addLayout(form);

#ifdef __APPLE__
	// The Qt host consumes both of these (transparent titlebar / blur).
	layout->addWidget(makeSection("macOS"));
	{
		auto *macForm = new QFormLayout();
		macForm->setSpacing(8);
		opacitySlider = new QSlider(Qt::Horizontal, this);
		opacitySlider->setRange(0, 100);
		opacitySlider->setValue(
			qRound(appSettings.settings.value("mac_background_opacity", 0.5f) * 100.0f));
		auto *opacityRow = new QWidget(this);
		auto *opacityLayout = new QHBoxLayout(opacityRow);
		opacityLayout->setContentsMargins(0, 0, 0, 0);
		opacityLayout->setSpacing(10);
		auto *opacityLabel =
			new QLabel(QString("%1%").arg(opacitySlider->value()), opacityRow);
		opacityLabel->setMinimumWidth(36);
		connect(opacitySlider, &QSlider::valueChanged, this, [opacityLabel](int v) {
			opacityLabel->setText(QString("%1%").arg(v));
		});
		opacityLayout->addWidget(opacitySlider, 1);
		opacityLayout->addWidget(opacityLabel);
		macForm->addRow("Background opacity", opacityRow);

		blurBox = new QCheckBox("Enable background blur", this);
		blurBox->setChecked(appSettings.settings.value("mac_blur_enabled", true));
		macForm->addRow("", blurBox);
		layout->addLayout(macForm);
	}
#endif

	// Editor toggles — same set as the ImGui window (minus the ImGui-only
	// sidebar/terminal rows and the GL shader section).
	layout->addWidget(makeSection("Toggles"));
	{
		auto *toggles = new QGridLayout();
		toggles->setHorizontalSpacing(12);
		toggles->setVerticalSpacing(6);

		auto addToggle = [&](int row,
							 int col,
							 const QString &text,
							 bool checked,
							 const QString &hint,
							 QCheckBox **out) {
			auto *box = new QCheckBox(text, this);
			box->setChecked(checked);
			box->setToolTip(hint);
			*out = box;
			toggles->addWidget(box, row, col);
		};
		addToggle(0,
				  0,
				  "Minimap",
				  appSettings.settings.value("minimap", true),
				  "Code overview strip on the right",
				  &minimapBox);
		addToggle(0,
				  1,
				  "Word wrap",
				  appSettings.settings.value("word_wrap", false),
				  "Wrap long lines to the window width",
				  &wordWrapBox);
		addToggle(1,
				  0,
				  "Tree-sitter highlighting",
				  appSettings.settings.value("treesitter", true),
				  "Syntax highlighting",
				  &treeSitterBox);
		addToggle(2,
				  0,
				  "Git changed-line markers",
				  appSettings.settings.value("git_changed_lines", true),
				  "Highlight changed lines in git",
				  &gitGutterBox);
		// Panels (ImGui "Toggle Settings" parity): file explorer +
		// terminal, applied live rather than staged for OK.
		addToggle(1,
				  1,
				  "File explorer",
				  appSettings.settings.value("sidebar_visible", true),
				  "Show/hide file explorer sidebar",
				  &sidebarBox);
		addToggle(2,
				  1,
				  "Terminal",
				  appSettings.settings.value("terminal_visible", true),
				  "Show/hide bottom terminal panel",
				  &terminalBox);
		auto panelsChanged = [this] {
			Q_EMIT panelTogglesChanged(sidebarBox->isChecked(), terminalBox->isChecked());
		};
		connect(sidebarBox, &QCheckBox::toggled, this, panelsChanged);
		connect(terminalBox, &QCheckBox::toggled, this, panelsChanged);
		layout->addLayout(toggles);
	}

	// Language servers — same surface the ImGui settings window exposes.
	layout->addWidget(makeSection("Language"));
	{
		auto *lspRow = new QWidget(this);
		auto *lspLayout = new QHBoxLayout(lspRow);
		lspLayout->setContentsMargins(0, 0, 0, 0);
		auto *label = new QLabel("Servers, status and lsp.json", lspRow);
		lspLayout->addWidget(label, 1);
		auto *dashboardButton = new QPushButton("Language Servers…", lspRow);
		connect(dashboardButton, &QPushButton::clicked, this, [this] {
			Q_EMIT lspDashboardRequested();
		});
		lspLayout->addWidget(dashboardButton);
		layout->addWidget(lspRow);
	}

	layout->addStretch(1);

	// Live-apply (ImGui parity): every control commits the moment it
	// changes — there is nothing to confirm or cancel, just a way out.
	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	layout->addWidget(buttons);

	// Wire live commits last, after the initial values are in place.
	syncing = true;
	syncFromSettings();
	syncing = false;
	connect(themeBox, &QComboBox::currentTextChanged, this, [this](const QString &) {
		if (!syncing)
			onProfileChanged();
	});
	for (auto *combo : {static_cast<QComboBox *>(fontBox)})
		connect(combo, &QComboBox::currentTextChanged, this, [this](const QString &) {
			if (!syncing)
				commit();
		});
	// While the handle is down, only update the readout; committing would
	// re-apply the font and relayout the whole UI on every drag tick.
	connect(fontSizeSlider, &QSlider::valueChanged, this, [this](int) {
		if (!syncing && !fontSizeSlider->isSliderDown())
			commit();
	});
	connect(fontSizeSlider, &QSlider::sliderReleased, this, [this]() {
		if (!syncing)
			commit();
	});
	connect(fontSizeBox, &QSpinBox::valueChanged, this, [this](int) {
		if (!syncing && !fontSizeSlider->isSliderDown())
			commit();
	});
#ifdef __APPLE__
	connect(opacitySlider, &QSlider::valueChanged, this, [this](int) {
		if (!syncing)
			commit();
	});
	connect(blurBox, &QCheckBox::toggled, this, [this](bool) {
		if (!syncing)
			commit();
	});
#endif
	for (QCheckBox *box :
		 {minimapBox, wordWrapBox, treeSitterBox, gitGutterBox, sidebarBox, terminalBox})
		connect(box, &QCheckBox::toggled, this, [this](bool) {
			if (!syncing)
				commit();
		});
}

QWidget *QtSettingsDialog::makeSection(const QString &title)
{
	auto *section = new QWidget(this);
	auto *column = new QVBoxLayout(section);
	column->setContentsMargins(0, 8, 0, 0);
	column->setSpacing(6);
	auto *header = new QLabel(title, section);
	header->setStyleSheet("font-weight: 600;");
	column->addWidget(header);
	auto *line = new QFrame(section);
	line->setFixedHeight(1);
	line->setStyleSheet("background: rgba(128,128,128,80); border: none;");
	column->addWidget(line);
	return section;
}

QPushButton *QtSettingsDialog::makeColorSwatch(const QColor &color)
{
	auto *swatch = new QPushButton(this);
	swatch->setCursor(Qt::PointingHandCursor);
	swatch->setToolTip("Click to pick a color");
	// Geometry in code, not QSS: the app sheet's QPushButton padding would
	// otherwise inflate every swatch by a different amount depending on
	// which rules win. Scales with the app font (13pt base) so a zoomed
	// font doesn't tower over the chip.
	const qreal z = qApp->font().pointSize() > 0 ? qApp->font().pointSize() / 13.0 : 1.0;
	swatch->setFixedSize(std::max(64, qRound(64 * z)), std::max(20, qRound(20 * z)));
	paintSwatch(swatch, color);
	return swatch;
}

void QtSettingsDialog::paintSwatch(QPushButton *swatch, const QColor &color)
{
	swatch->setStyleSheet(
		QString(
			"background: rgba(%1,%2,%3,%4); border: 1px solid rgba(255,255,255,0.25); "
			"border-radius: 4px; padding: 0; margin: 0;")
			.arg(color.red())
			.arg(color.green())
			.arg(color.blue())
			.arg(color.alpha()));
}

void QtSettingsDialog::commit()
{
	// Persist the FILE STEM, not the combo's display family: the ImGui
	// host loads fonts as "<stem>.ttf" — a family name ("Source Code Pro")
	// would not resolve there and ImGui would fall back to no font.
	const QString family = fontBox->currentText();
	appSettings.settings["font"] =
		family == QLatin1String("System Default")
			? std::string("System Default")
			: NedQtFonts::resolveStem(family).toStdString();
	appSettings.settings["fontSize"] = fontSizeBox->value();
#ifdef __APPLE__
	appSettings.settings["mac_background_opacity"] = opacitySlider->value() / 100.0f;
	appSettings.settings["mac_blur_enabled"] = blurBox->isChecked();
#endif
	appSettings.settings["backgroundColor"] = {
		pendingBg.redF(), pendingBg.greenF(), pendingBg.blueF(), pendingBg.alphaF()};
	appSettings.settings["minimap"] = minimapBox->isChecked();
	appSettings.settings["word_wrap"] = wordWrapBox->isChecked();
	appSettings.settings["treesitter"] = treeSitterBox->isChecked();
	appSettings.settings["git_changed_lines"] = gitGutterBox->isChecked();
	appSettings.settings["sidebar_visible"] = sidebarBox->isChecked();
	appSettings.settings["terminal_visible"] = terminalBox->isChecked();
	appSettings.saveSettings();
	// Views read the profile live (colors via HighlightService, font via
	// host); the Qt host re-applies on the applied() signal.
	appSettings.requestApply();
	Q_EMIT applied();
}

void QtSettingsDialog::syncFromSettings()
{
	themeBox->setCurrentText(QString::fromStdString(appSettings.activeProfileFile()));
	stagedProfileFile = appSettings.activeProfileFile();
	// Font: the profile may store a bundled file stem — resolve to family.
	const QString stored = QString::fromStdString(
		appSettings.settings.value("font", std::string("System Default")));
	QString family = NedQtFonts::resolveFamily(stored);
	if (family.isEmpty())
		family = stored;
	fontBox->setCurrentText(fontBox->findText(family) >= 0 ? family : "System Default");
	const int size = static_cast<int>(appSettings.settings.value("fontSize", 13));
	fontSizeSlider->setValue(size);
	fontSizeBox->setValue(size);
	pendingBg = NedQtTheme::colorFromJson(appSettings.settings.contains("backgroundColor")
											  ? &appSettings.settings["backgroundColor"]
											  : nullptr,
										  NedQtTheme::defaultBackground());
	paintSwatch(bgButton, pendingBg);
#ifdef __APPLE__
	opacitySlider->setValue(static_cast<int>(
		appSettings.settings.value("mac_background_opacity", 1.0f) * 100));
	blurBox->setChecked(appSettings.settings.value("mac_blur_enabled", true));
#endif
	minimapBox->setChecked(appSettings.settings.value("minimap", true));
	wordWrapBox->setChecked(appSettings.settings.value("word_wrap", false));
	treeSitterBox->setChecked(appSettings.settings.value("treesitter", true));
	gitGutterBox->setChecked(appSettings.settings.value("git_changed_lines", true));
	sidebarBox->setChecked(appSettings.settings.value("sidebar_visible", true));
	terminalBox->setChecked(appSettings.settings.value("terminal_visible", true));
}

void QtSettingsDialog::onProfileChanged()
{
	// Switch reloads `settings` from the new profile JSON — resync every
	// control from it so the dialog now shows (and future commits write)
	// the new theme's values.
	appSettings.switchToProfile(themeBox->currentText().toStdString());
	syncing = true;
	syncFromSettings();
	syncing = false;
	appSettings.saveSettings();
	appSettings.requestApply();
	Q_EMIT applied();
}
