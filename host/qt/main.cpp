/*
	File: host/qt/main.cpp
	Description: Entry point for the Qt backend of ned.
*/

#include "qt_host.h"

#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	app.setApplicationName("Ned");

	NedQtHost window;
	window.show();

	return app.exec();
}
