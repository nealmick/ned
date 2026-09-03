#include "clipboard_qt.h"

#include <QClipboard>
#include <QGuiApplication>

void QtClipboard::setText(const std::string &text)
{
	QGuiApplication::clipboard()->setText(QString::fromStdString(text));
}

std::string QtClipboard::text() const
{
	return QGuiApplication::clipboard()->text().toStdString();
}
