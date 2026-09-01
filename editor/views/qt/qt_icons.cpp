#include "qt_icons.h"

#include "../../../util/icons.h"
#include "../../../util/settings.h"

#include <QFile>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>

#include <map>

QString QtIconSet::iconsDir() const
{
	return QString::fromStdString(Settings::getAppResourcesPath()) + "/resources/icons";
}

QtIconSet &QtIconSet::instance()
{
	static QtIconSet set;
	return set;
}

QIcon QtIconSet::byKey(const QString &key)
{
	static std::map<QString, QIcon> cache;

	auto it = cache.find(key);
	if (it != cache.end())
		return it->second;

	const QString path = iconsDir() + "/" + key + ".svg";
	QIcon icon;
	if (QFile::exists(path))
	{
		QSvgRenderer renderer(path);
		QPixmap pixmap(32, 32);
		pixmap.fill(Qt::transparent);
		QPainter painter(&pixmap);
		renderer.render(&painter);
		painter.end();
		icon = QIcon(pixmap);
	}
	cache.emplace(key, icon);
	return icon;
}

QIcon QtIconSet::forFile(const QString &filename)
{
	return byKey(QString::fromStdString(Icons::iconKeyForFile(filename.toStdString())));
}
