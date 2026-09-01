/*
	File: views/qt/qt_theme.h
	Description: Builds a Qt application palette from the shared settings
	profile (background + theme text), so the Qt chrome (sidebar, welcome,
	title bars) matches the editor's theme like the ImGui build.
*/

#pragma once

#include "../../../util/settings.h"

#include <QColor>
#include <QPalette>

namespace NedQtTheme {

// Editor-area background from the active profile.
inline QColor background(const Settings &s)
{
	if (s.settings.contains("backgroundColor") &&
		s.settings["backgroundColor"].size() >= 3)
	{
		const auto &bg = s.settings["backgroundColor"];
		return QColor::fromRgbF(bg[0].get<float>(), bg[1].get<float>(),
								bg[2].get<float>());
	}
	return QColor(0x1e, 0x1e, 0x1e);
}

// Active theme's text color (falls back to a light gray).
inline QColor text(const Settings &s)
{
	const std::string theme =
		s.settings.value("theme", std::string("default"));
	if (s.settings.contains("themes") && s.settings["themes"].contains(theme))
	{
		const auto &t = s.settings["themes"][theme]["text"];
		if (t.size() >= 3)
			return QColor::fromRgbF(t[0].get<float>(), t[1].get<float>(),
									t[2].get<float>());
	}
	return QColor(0xd0, 0xd0, 0xd0);
}

// Slightly lighter than the background — title bars, separators.
inline QColor raised(const QColor &bg)
{
	return QColor(std::min(255, bg.red() + 12), std::min(255, bg.green() + 12),
				  std::min(255, bg.blue() + 16));
}

inline QPalette palette(const Settings &s)
{
	const QColor bg = background(s);
	const QColor ink = text(s);
	QPalette p;
	p.setColor(QPalette::Window, bg);
	p.setColor(QPalette::Base, bg);
	p.setColor(QPalette::AlternateBase, raised(bg));
	p.setColor(QPalette::Text, ink);
	p.setColor(QPalette::WindowText, ink);
	p.setColor(QPalette::Button, raised(bg));
	p.setColor(QPalette::ButtonText, ink);
	p.setColor(QPalette::ToolTipBase, raised(bg));
	p.setColor(QPalette::ToolTipText, ink);
	p.setColor(QPalette::Highlight, QColor(255, 30, 170));
	p.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
	p.setColor(QPalette::PlaceholderText, ink.darker(140));
	return p;
}

} // namespace NedQtTheme
