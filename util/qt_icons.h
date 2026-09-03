/*
	File: util/qt_icons.h
	Description: Renders ned's own SVG icon set (resources/icons) into
	QIcons via QSvgRenderer. Shares Icons::iconKeyForFile with the ImGui
	backend so both show the same file-type icons. (Qt-side sibling of
	util/imgui_icons.* — per-backend icon glue lives here.)
*/

#pragma once

#include <QIcon>
#include <QString>

namespace QtIconSet {

// Icon for a filename (same key resolution as the ImGui backend).
QIcon forFile(const QString &filename, int px = 16);
// Icon by key ("folder", "folder-open", "cpp", ...). Renders at the
// requested pixel size so icons scale with the app font.
QIcon byKey(const QString &key, int px = 16);

} // namespace QtIconSet
