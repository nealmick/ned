#include "fonts.h"

#include "../../../util/settings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QtMath>

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

QFont terminalFont(const Settings &settings)
{
	// qtermwidget measures its cell width as an INTEGER (qRound of the
	// average advance) but paints glyphs at sub-pixel advances: at point
	// sizes whose advance is fractional (e.g. Source Code Pro 19pt =
	// 11.39px -> cell 11px) every glyph paints ~0.4px wider than its cell
	// and the cursor drifts LEFT of the typed text, accumulating per
	// column. Kerning off + snapping the point size to the nearest size
	// with an INTEGRAL advance makes painted advance == measured cell
	// exactly, with no change to the vendored widget.
	QFont font = profileMonoFont(settings);
	font.setKerning(false);

	const int want = font.pointSize();
	for (int off : {0, 1, -1, 2, -2})
	{
		const int pt = want + off;
		if (pt < 8 || pt > 40)
			continue;
		QFont probe = font;
		probe.setPointSize(pt);
		const qreal adv = QFontMetricsF(probe).horizontalAdvance(QLatin1Char('M'));
		if (qAbs(adv - qRound(adv)) < 0.05)
		{
			font.setPointSize(pt);
			break;
		}
	}
	return font;
}

} // namespace NedQtFonts
