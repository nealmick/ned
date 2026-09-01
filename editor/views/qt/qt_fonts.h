/*
	File: views/qt/qt_fonts.h
	Description: Profile font names are bundled file stems ("SourceCodePro-
	Regular"); Qt needs family names. This maps stems to the families
	registered from resources/fonts (host registers them at startup).
*/

#pragma once

#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHash>
#include <QString>

namespace NedQtFonts {

inline QHash<QString, QString> &stemToFamily()
{
	static QHash<QString, QString> map;
	return map;
}

// Call once after addApplicationFont registrations: remembers
// file-stem -> real family.
inline void registerFontFile(const QString &filePath, int appFontId)
{
	if (appFontId < 0)
		return;
	const QString stem = QFileInfo(filePath).completeBaseName();
	for (const QString &family : QFontDatabase::applicationFontFamilies(appFontId))
	{
		stemToFamily()[stem.toLower()] = family;
		stemToFamily()[family.toLower()] = family; // family lookups too
	}
}

// Resolve a profile font setting to a real Qt family (empty when unknown).
inline QString resolveFamily(const QString &setting)
{
	return stemToFamily().value(setting.toLower());
}

} // namespace NedQtFonts
