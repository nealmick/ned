/*
	File: host/qt/main.cpp
	Description: Entry point for the Qt backend of ned.
*/

#include "host.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
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

	AppHost window;
	window.show();

	// Files from the command line; otherwise the welcome screen (already
	// installed by the host constructor) is showing. Workspace FIRST: git
	// and the language server both need the root before the first document
	// opens (a file opened before its workspace would never start LSP —
	// init() refuses without a workspace).
	QCommandLineParser args;
	args.process(*QApplication::instance());
	const QStringList positional = args.positionalArguments();
	if (!positional.isEmpty())
		window.openWorkspace(QFileInfo(positional.first()).absolutePath());
	for (const QString &path : positional)
		window.openPath(path, false);

	return app.exec();
}
