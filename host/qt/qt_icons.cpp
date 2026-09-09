#include "qt_icons.h"

#include "../../util/icon_keys.h"
#include "../../util/settings.h"

#include <QApplication>
#include <QFile>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>
#include <algorithm>
#include <cmath>

#include <map>

namespace {
QString iconsDir()
{
	return QString::fromStdString(Settings::getAppResourcesPath()) + "/resources/icons";
}
} // namespace

QIcon QtIconSet::byKey(const QString &key, int px)
{
	static std::map<QString, std::map<int, QIcon>> cache;

	auto &sizeCache = cache[key];
	auto it = sizeCache.find(px);
	if (it != sizeCache.end())
		return it->second;

	const QString path = iconsDir() + "/" + key + ".svg";
	QIcon icon;
	if (QFile::exists(path))
	{
		// Normalize by INK, not viewBox: the bundled SVGs have wildly
		// different baked-in padding (folders fill the canvas, language
		// icons don't), so equal raster sizes look unequal. Render big,
		// measure the opaque bounding box, then scale so every icon's ink
		// fills the same fraction of the final square.
		QSvgRenderer renderer(path);
		QImage source(128, 128, QImage::Format_ARGB32);
		source.fill(Qt::transparent);
		{
			QPainter sp(&source);
			renderer.render(&sp);
		}
		int minX = 128, minY = 128, maxX = -1, maxY = -1;
		for (int y = 0; y < 128; ++y)
			for (int x = 0; x < 128; ++x)
				if (qAlpha(source.pixel(x, y)) > 16)
				{
					if (x < minX)
						minX = x;
					if (x > maxX)
						maxX = x;
					if (y < minY)
						minY = y;
					if (y > maxY)
						maxY = y;
				}
		QPixmap pixmap(qRound(px * 2.0), qRound(px * 2.0));
		pixmap.fill(Qt::transparent);
		if (maxX >= minX && maxY >= minY)
		{
			// Ink box scaled to ~88% of the target square, centered.
			const qreal inkW = maxX - minX + 1, inkH = maxY - minY + 1;
			const qreal target = px * 2.0 * 0.88;
			const qreal scale = std::min(target / inkW, target / inkH);
			const qreal w = inkW * scale, h = inkH * scale;
			QPainter painter(&pixmap);
			painter.setRenderHint(QPainter::SmoothPixmapTransform);
			const QRectF dest((px * 2.0 - w) / 2.0, (px * 2.0 - h) / 2.0, w, h);
			const QRectF src(minX, minY, inkW, inkH);
			painter.drawImage(dest, source, src);
			painter.end();
		}
		// The raster is a true 2x asset for the requested logical size — say
		// so, or item views on a retina screen rescale the DPR-1 pixmap
		// themselves (upscaled folders look pixelated).
		pixmap.setDevicePixelRatio(2.0);
		icon = QIcon(pixmap);
	}
	sizeCache.emplace(px, icon);
	return icon;
}

QIcon QtIconSet::forFile(const QString &filename, int px)
{
	return byKey(QString::fromStdString(iconKeyForFile(filename.toStdString())), px);
}
