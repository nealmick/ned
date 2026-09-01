/*
	File: host/qt/main.cpp
	Description: Entry point for the Qt backend of ned.
*/

#include "editor/views/qt/qt_editor_view.h"
#include "qt_host.h"

#include <QApplication>
#include <QImage>
#include <QKeyEvent>
#include <QWidget>
#include <chrono>
#include <cstdlib>
#include <iostream>

int main(int argc, char *argv[])
{
	for (int i = 1; i < argc; ++i)
		if (std::string(argv[i]) == "--version")
		{
			std::cout << "ned_qt " << NED_QT_BUILD_STAMP << std::endl;
			return 0;
		}

	QApplication app(argc, argv);
	app.setApplicationName("Ned");

	NedQtHost window;
	window.show();

	// Headless render check: grab the window, count lit pixels, bail.
	// Usage: NED_QT_RENDER_CHECK=1 ned_qt <file>
	if (qEnvironmentVariableIsSet("NED_QT_RENDER_CHECK"))
	{
		const auto t0 = std::chrono::steady_clock::now();
		for (int i = 0; i < 20; ++i)
		{
			app.processEvents();
			window.repaint();
			if (qApp->styleSheet().isEmpty())
			{
				break;
			}
		}
		const auto paintMs = std::chrono::duration_cast<std::chrono::milliseconds>(
								 std::chrono::steady_clock::now() - t0)
								 .count() /
							 20.0;
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
				  << " lit=" << lit << " paintMs=" << paintMs << std::endl;
		grab.save("/tmp/ned_qt_grab.png");
		return 0;
	}

	// Test hook: reproduce the user's project-open flow without GUI
	// driving — open workspace + file, then measure input latency.
	if (qEnvironmentVariableIsSet("NED_QT_WORKSPACE"))
	{
		window.openWorkspace(QString::fromUtf8(qgetenv("NED_QT_WORKSPACE")));
		window.openPath(QString::fromUtf8(qgetenv("NED_QT_FILE")), true);
	}

	// Synthetic interaction test: open editor, hammer keys, time latency.
	if (qEnvironmentVariableIsSet("NED_QT_INTERACT"))
	{
		QWidget *editor = window.findChild<QWidget *>("__ned_editor");
		if (!editor)
		{
			for (QWidget *w : window.findChildren<QWidget *>())
				if (w->metaObject()->className() == QByteArray("QtEditorView"))
				{
					editor = w;
					break;
				}
		}
		if (editor)
		{
			editor->setFocus();
			const auto t0 = std::chrono::steady_clock::now();
			for (int i = 0; i < 100; ++i)
			{
				QApplication::postEvent(
					editor,
					new QKeyEvent(QEvent::KeyPress, Qt::Key_X, Qt::NoModifier, "x"));
				app.processEvents(); // one full input->command->paint cycle
			}
			const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
								std::chrono::steady_clock::now() - t0)
								.count();
			std::cerr << "[interact] 100 keys in " << ms << "ms (" << ms / 100.0
					  << "ms/key)" << std::endl;
			if (auto *ed = qobject_cast<QtEditorView *>(editor))
				ed->debugMinimapBottom();
			// Git gutter check: after edits, dirty lines + summary must update.
			if (auto *ed = qobject_cast<QtEditorView *>(editor))
				std::cerr << "[interact] git dirty lines=" << ed->gitDirtyLineCount()
						  << " summary='" << ed->gitChangesSummary() << "'" << std::endl;
		} else
			std::cerr << "[interact] no editor found" << std::endl;
		return 0;
	}

	return app.exec();
}
