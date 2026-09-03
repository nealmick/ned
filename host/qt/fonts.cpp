#include "fonts.h"

#include "../../../util/settings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>

namespace NedQtFonts {

QHash<QString, QString> &stemToFamily()
{
	static QHash<QString, QString> map;
	return map;
}

QHash<QString, QString> &familyToStem()
{
	static QHash<QString, QString> map;
	return map;
}

void registerFontFile(const QString &filePath, int appFontId)
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

void registerBundledFonts()
{
	const QDir fontsDir(QString::fromStdString(Settings::getAppResourcesPath()) +
						"/resources/fonts");
	for (const QString &file : fontsDir.entryList({"*.ttf", "*.otf"}, QDir::Files))
		registerFontFile(fontsDir.filePath(file),
						 QFontDatabase::addApplicationFont(fontsDir.filePath(file)));
}

QString resolveFamily(const QString &setting)
{
	return stemToFamily().value(setting.toLower());
}

QString resolveStem(const QString &family)
{
	return familyToStem().value(family.toLower());
}

QFont profileMonoFont(const Settings &settings)
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
