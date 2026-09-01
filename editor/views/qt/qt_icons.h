/*
	File: views/qt/qt_icons.h
	Description: Renders ned's own SVG icon set (resources/icons) into
	QIcons via QSvgRenderer. Shares Icons::iconKeyForFile with the ImGui
	backend so both show the same file-type icons.
*/

#pragma once

#include <QIcon>
#include <QString>

class QtIconSet
{
  public:
	static QtIconSet &instance();

	// Icon for a filename (same key resolution as the ImGui backend).
	QIcon forFile(const QString &filename);
	// Icon by key ("folder", "folder-open", "cpp", ...).
	QIcon byKey(const QString &key);

  private:
	QString iconsDir() const;
};
