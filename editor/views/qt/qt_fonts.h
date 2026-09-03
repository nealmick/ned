/*
	File: views/qt/qt_fonts.h
	Description: Profile font names are bundled file stems ("SourceCodePro-
	Regular"); Qt needs family names. This maps stems to the families
	registered from resources/fonts (host registers them at startup).
*/

#pragma once

#include "../../../util/settings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QHash>
#include <QString>

namespace NedQtFonts {

inline QHash<QString, QString> &stemToFamily()
{
	static QHash<QString, QString> map;
	return map;
}

inline QHash<QString, QString> &familyToStem()
{
	static QHash<QString, QString> map;
	return map;
}

// Call once after addApplicationFont registrations: remembers
// file-stem -> real family (and back — the settings dialog persists
// STEMS: the ImGui host loads fonts by "<stem>.ttf").
inline void registerFontFile(const QString &filePath, int appFontId)
{
	if (appFontId < 0)
		return;
	const QString stem = QFileInfo(filePath).completeBaseName();
	for (const QString &family : QFontDatabase::applicationFontFamilies(appFontId))
	{
		stemToFamily()[stem.toLower()] = family;
		stemToFamily()[family.toLower()] = family; // family lookups too
		familyToStem()[family.toLower()] = stem;
	}
}

// Register every bundled font in resources/fonts — the ONE registration
// site (the Qt host's startup). Later callers (settings dialog) only
// READ the registered families; re-registering would duplicate them.
inline void registerBundledFonts()
{
	const QDir fontsDir(QString::fromStdString(Settings::getAppResourcesPath()) +
						"/resources/fonts");
	for (const QString &file : fontsDir.entryList({"*.ttf", "*.otf"}, QDir::Files))
		registerFontFile(fontsDir.filePath(file),
						 QFontDatabase::addApplicationFont(fontsDir.filePath(file)));
}

// Resolve a profile font setting to a real Qt family (empty when unknown).
inline QString resolveFamily(const QString &setting)
{
	return stemToFamily().value(setting.toLower());
}

// Reverse: the profile file stem for a Qt family (empty when unknown) —
// what gets written back to the profile so the ImGui host (which loads
// "<stem>.ttf") still finds the font.
inline QString resolveStem(const QString &family)
{
	return familyToStem().value(family.toLower());
}

// The monospace editor/terminal font from the profile — the single
// resolution rule (profile family when registered, Menlo/Consolas
// fallback, always fixed-pitch). Editor view and terminal panel share
// it; qtermwidget derives its cell width from the font, so a
// proportional family would draw glyphs over each other.
inline QFont profileMonoFont(const Settings &settings)
{
	QFont font("Menlo");
#ifdef _WIN32
	font.setFamily("Consolas");
#endif
	const std::string profileFont = settings.settings.value("font", std::string());
	const QString resolved = resolveFamily(QString::fromStdString(profileFont));
	if (!profileFont.empty() && profileFont != "System Default" && !resolved.isEmpty())
		font.setFamily(resolved);
	font.setStyleHint(QFont::Monospace);
	font.setFixedPitch(true);
	font.setPointSize(static_cast<int>(settings.settings.value("fontSize", 13)));
	return font;
}

} // namespace NedQtFonts
