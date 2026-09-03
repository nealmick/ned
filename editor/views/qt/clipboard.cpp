#include "clipboard.h"

#include <QClipboard>
#include <QGuiApplication>

void Clipboard::setText(const std::string &text)
{
	QGuiApplication::clipboard()->setText(QString::fromStdString(text));
}

std::string Clipboard::text() const
{
	return QGuiApplication::clipboard()->text().toStdString();
}
