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
		return QColor::fromRgbF(
			bg[0].get<float>(), bg[1].get<float>(), bg[2].get<float>());
	}
	return QColor(0x1e, 0x1e, 0x1e);
}

// Active theme's text color (falls back to a light gray).
inline QColor text(const Settings &s)
{
	const std::string theme = s.settings.value("theme", std::string("default"));
	if (s.settings.contains("themes") && s.settings["themes"].contains(theme))
	{
		const auto &t = s.settings["themes"][theme]["text"];
		if (t.size() >= 3)
			return QColor::fromRgbF(
				t[0].get<float>(), t[1].get<float>(), t[2].get<float>());
	}
	return QColor(0xd0, 0xd0, 0xd0);
}

// Slightly lighter than the background — title bars, separators.
inline QColor raised(const QColor &bg)
{
	return QColor(std::min(255, bg.red() + 12),
				  std::min(255, bg.green() + 12),
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
	// Selection/accent: bootstrap-like blue (#0d6efd), softer for chrome.
	p.setColor(QPalette::Highlight, QColor(0x0d, 0x6e, 0xfd));
	p.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
	p.setColor(QPalette::PlaceholderText, ink.darker(140));
	return p;
}

// Rounded styling throughout; all tabs keep identical geometry (the
// active tab changes color, never size).
inline QString appStyleSheet(const Settings &s)
{
	const QString bg = background(s).name();
	const QString raised = NedQtTheme::raised(background(s)).name();
	const QString ink = text(s).name();
	const QString accent = QStringLiteral("#0d6efd");
	// Modern flat macOS styling: pill tabs with FIXED geometry (the
	// selected tab changes color only), hairline borders, no bevels.
	return QStringLiteral(R"(
		QTabWidget::pane { border: none; border-top: 1px solid rgba(255,255,255,0.06); }
		QTabBar { background: transparent; spacing: 2px; }
		QTabBar::tab {
			padding: 4px 12px;
			min-height: 24px; max-height: 24px;
			margin: 4px 1px 2px 1px;
			border: none;
			border-radius: 6px;
			background: rgba(255,255,255,0.045);
			color: %3;
		}
		QTabBar::tab:selected { background: rgba(255,255,255,0.13); color: %4; }
		QTabBar::tab:hover:!selected { background: rgba(255,255,255,0.08); }
		QToolButton {
			border: none; border-radius: 5px;
			padding: 1px 5px;
			background: transparent; color: %3;
			font-size: 11px;
		}
		QToolButton:hover { background: rgba(255,255,255,0.14); }
		QLineEdit, QSpinBox, QComboBox {
			border: 1px solid rgba(255,255,255,0.08);
			border-radius: 7px;
			padding: 4px 8px;
			background: rgba(255,255,255,0.04);
			color: %3;
			selection-background-color: %5;
		}
		QLineEdit:focus { border-color: %5; }
		QTreeWidget, QListWidget, QTreeView, QListView {
			border: none; background: transparent; color: %3;
			outline: none;
		}
		QTreeWidget::item { border-radius: 5px; padding: 2px; }
		/* Branch/indicator column: transparent in every state, else it
		   paints its own hover box next to the item's (double hover). */
		QTreeView::branch,
		QTreeView::branch:hover,
		QTreeView::branch:selected,
		QTreeView::branch:has-children:hover { background: transparent; border: none; }
		QTreeWidget::item:hover { background: rgba(255,255,255,0.06); }
		QTreeWidget::item:selected { background: rgba(13,110,253,0.32); }
		QListWidget::item { border-radius: 5px; padding: 3px 6px; }
		QListWidget::item:selected { background: rgba(13,110,253,0.32); }
		QPushButton {
			border: none; border-radius: 7px;
			padding: 6px 16px;
			background: rgba(255,255,255,0.09); color: %3;
		}
		QPushButton:hover { background: rgba(255,255,255,0.15); }
		QPushButton:pressed { background: rgba(255,255,255,0.22); }
		QMenu { border: 1px solid rgba(255,255,255,0.08); border-radius: 9px; background: %2; color: %3; padding: 5px; }
		QMenu::item { padding: 5px 26px 5px 14px; border-radius: 5px; }
		QMenu::item:selected { background: rgba(13,110,253,0.4); }
		QLabel { color: %3; background: transparent; }
		QScrollBar:vertical, QScrollBar:horizontal { width: 0; height: 0; }
		QDockWidget { titlebar-close-icon: none; titlebar-normal-icon: none; }
	)")
		// NOTE: the template no longer uses %1 — pass only %2..%5 in order,
		// or QString::arg shifts every color by one (the dark-on-dark bug).
		.arg(raised, ink, ink, accent);
	(void)bg;
}

} // namespace NedQtTheme
