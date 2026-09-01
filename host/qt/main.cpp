/*
	File: host/qt/main.cpp
	Description: Entry point for the Qt backend of ned.
*/

#include "qt_host.h"

#include <QApplication>
#include <QImage>
#include <cstdlib>
#include <iostream>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	app.setApplicationName("Ned");

	NedQtHost window;
	window.show();

	// Headless render check: grab the window, count lit pixels, bail.
	// Usage: NED_QT_RENDER_CHECK=1 ned_qt <file>
	if (qEnvironmentVariableIsSet("NED_QT_RENDER_CHECK"))
	{
		for (int i = 0; i < 20; ++i)
		{
			app.processEvents();
			window.repaint();
		}
		const QImage grab = window.grab().toImage();
		// Count pixels that differ from the editor background (corner):
		// gutter numbers, text glyphs, caret, minimap strip.
		// Text band only: x in (120, width-100), y in (60, height-20) —
		// excludes gutter numbers and the minimap strip.
		const QRgb bg = grab.pixel(grab.width() / 2, grab.height() - 10);
		int lit = 0;
		for (int y = 60; y < grab.height() - 20; ++y)
			for (int x = 120; x < grab.width() - 100; ++x)
			{
				const QRgb px = grab.pixel(x, y);
				if (std::abs(qRed(px) - qRed(bg)) + std::abs(qGreen(px) - qGreen(bg)) +
						std::abs(qBlue(px) - qBlue(bg)) >
					40)
					++lit;
			}
		std::cerr << "[render-check] " << grab.width() << "x" << grab.height()
				  << " lit=" << lit << std::endl;
		grab.save("/tmp/ned_qt_grab.png");
		return 0;
	}

	return app.exec();
}
