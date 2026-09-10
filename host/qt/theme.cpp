/*
	File: host/qt/theme.cpp
	Description: Definitions for the Qt theme palette (theme.h).
*/

#include "theme.h"

#include "../../../util/settings.h"

#include <QApplication>
#include <QColor>
#include <QPalette>

namespace NedQtTheme {

QColor colorFromJson(const json *arr, const QColor &fallback)
{
	if (!arr || !arr->is_array() || arr->size() < 3)
		return fallback;
	return QColor::fromRgbF((*arr)[0].get<float>(),
							(*arr)[1].get<float>(),
							(*arr)[2].get<float>(),
							arr->size() >= 4 ? (*arr)[3].get<float>() : 1.0f);
}

QColor defaultBackground() { return QColor(0x1e, 0x1e, 0x1e); }

// Editor-area background from the active profile. On macOS the alpha
// carries the background-opacity setting: every base paint (palette,
// editor fill, title strip) becomes translucent so the window vibrancy
// layer shows through — the ImGui build's frosted-glass look. QSS colors
// go through QColor::name() (#RRGGBB), which DROPS alpha — menus and
// tooltips stay opaque. Non-Apple platforms get a fully opaque color.
QColor background(const Settings &s)
{
	QColor rgb = colorFromJson(
		s.settings.contains("backgroundColor") ? &s.settings["backgroundColor"] : nullptr,
		defaultBackground());
#ifdef __APPLE__
	const float alpha = s.settings.value("mac_background_opacity", 0.5f);
	return QColor::fromRgbF(rgb.redF(), rgb.greenF(), rgb.blueF(), alpha);
#else
	return QColor::fromRgbF(rgb.redF(), rgb.greenF(), rgb.blueF());
#endif
}

QColor text(const Settings &s)
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

QColor raised(const QColor &bg)
{
	return QColor(std::min(255, bg.red() + 12),
				  std::min(255, bg.green() + 12),
				  std::min(255, bg.blue() + 16));
}

QString popoverColor()
{
	const QColor bg = qApp->palette().color(QPalette::Window);
	return raised(QColor(bg.red(), bg.green(), bg.blue())).name();
}

QString popoverCardSheet(int radius)
{
	return QStringLiteral("#nedPopoverCard { background: %1; "
						  "border: 1px solid rgba(128, 128, 128, 110); "
						  "border-radius: %2; }")
		.arg(popoverColor())
		.arg(radius);
}

QPalette palette(const Settings &s)
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
	// TEXT selection keeps the classic blue highlight; the chrome accent
	// (sliders, focus rings, tree/menu tints) is the neutral grey in the
	// stylesheet — sliders/checkboxes are QSS-styled off the grey, so this
	// blue role only reaches actual text selection.
	p.setColor(QPalette::Highlight, QColor(0x0d, 0x6e, 0xfd));
	p.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
	p.setColor(QPalette::PlaceholderText, ink.darker(140));
	return p;
}

QString appStyleSheet(const Settings &s, int fontPt, int monoPt)
{
	const QString raised = NedQtTheme::raised(background(s)).name();
	const QString ink = text(s).name();
	const QString accent = QStringLiteral("#a9aeb6");
	// Tab chrome scales with the app font (13pt = base profile size):
	// px-fixed pills clip their label once the font zooms past them.
	// Platform split: macOS keeps the original compact pills; Windows
	// (custom title bar) uses the taller pills with real padding.
	const qreal z = fontPt / 13.0;
#ifdef __APPLE__
	const int tabH = std::max(16, qRound(18 * z));
	const int tabPadV = std::max(0, qRound(1 * z));
	const int tabPadH = std::max(4, qRound(6 * z));
	const int tabMTop = std::max(2, qRound(2 * z));
	const int tabMBot = std::max(1, qRound(1 * z));
#else
	const int tabH = std::max(22, qRound(26 * z));
	const int tabPadV = std::max(2, qRound(3 * z));
	const int tabPadH = std::max(6, qRound(9 * z));
	const int tabMTop = std::max(3, qRound(4 * z));
	const int tabMBot = std::max(3, qRound(4 * z));
#endif
	const int tabMSide = std::max(1, qRound(1 * z));
	const int tabR = std::max(4, qRound(5 * z));
	// Modern flat macOS styling: pill tabs with FIXED geometry (the
	// selected tab changes color only), hairline borders, no bevels.
	// App font travels IN the stylesheet: QApplication::setFont does not
	// propagate to existing widgets while a stylesheet is active (the
	// sidebar font lagged behind on every zoom).
	return QStringLiteral(R"(
		* { font-size: %6pt; }
		/* The universal font-size rule above also hits the terminal's
		   QTermWidget children: Qt resolves the stylesheet font OVER the
		   setTerminalFont font, so glyphs render at the app font size while
		   qtermwidget computes its cell width from the terminal font — the
		   cursor then sits left of the typed glyphs. Pin the terminal
		   subtree to the terminal font's own size. */
		#terminalTabs, #terminalTabs * { font-size: %14pt; }
		QTabWidget::pane { border: none; border-top: 1px solid rgba(255,255,255,0.09); }
		/* Terminal panel: the splitter handle is the border — the pane's
		   hairline would double it (dark line on light themes). */
		#terminalTabs::pane { border: none; }
		QTabBar { background: transparent; spacing: 2px; }
		QTabBar::tab {
			padding: %7px %8px;
			min-height: %9px; max-height: %9px;
			margin: %10px %11px %12px %11px;
			border: none;
			border-radius: %13px;
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
			selection-background-color: #0d6efd;
			selection-color: #ffffff;
		}
		QLineEdit:focus { border-color: %5; }
		/* Sliders: flat grey — thin groove, tinted sub-page, round handle.
		   Explicit rules keep macOS from drawing the system-blue slider. */
		QSlider::groove:horizontal { height: 4px; border-radius: 2px; background: rgba(255,255,255,0.14); }
		QSlider::sub-page:horizontal { border-radius: 2px; background: rgba(169,174,182,0.55); }
		QSlider::handle:horizontal { width: 14px; height: 14px; margin: -6px 0; border-radius: 7px; background: %5; }
		QSlider::handle:horizontal:hover { background: #c2c7cf; }
		/* Checkboxes: same flat-grey language — checked is a solid accent
		   fill (no glyph needed; macOS would otherwise paint system blue). */
		QCheckBox::indicator { width: 16px; height: 16px; border-radius: 5px; border: 1px solid rgba(255,255,255,0.25); background: rgba(255,255,255,0.05); }
		QCheckBox::indicator:hover { border-color: rgba(255,255,255,0.4); }
		QCheckBox::indicator:checked { background: %5; border-color: %5; }
		/* Hairline separator between the file tree and the editor. */
		#FileSidebar { border-right: 1px solid rgba(255,255,255,0.09); }
		QTreeWidget, QListWidget, QTreeView, QListView {
			border: none; background: transparent; color: %3;
			outline: none;
		}
		/* File-tree rows paint their own highlight: ONE rounded pill per
		   full row (branch arrow + label) drawn in FileSidebarView::paintEvent.
		   The tree runs NoSelection so the base style's opaque palette
		   highlight never fires; per-cell QSS backgrounds would split the
		   row into two boxes once the corners are rounded. */
		QTreeWidget::item { background: transparent; border: none; padding: 2px; }
		QTreeView::branch,
		QTreeView::branch:hover,
		QTreeView::branch:selected,
		QTreeView::branch:has-children:hover { background: transparent; border: none; }
		QListWidget::item { border-radius: 5px; padding: 3px 6px; }
		QListWidget::item:selected { background: rgba(169,174,182,0.32); }
		QPushButton {
			border: none; border-radius: 7px;
			padding: 6px 16px;
			background: rgba(255,255,255,0.09); color: %3;
		}
		QPushButton:hover { background: rgba(255,255,255,0.15); }
		QPushButton:pressed { background: rgba(255,255,255,0.22); }
		QMenu { border: 1px solid rgba(255,255,255,0.08); border-radius: 9px; background: %2; color: %3; padding: 5px; }
		QMenu::item { padding: 5px 26px 5px 14px; border-radius: 5px; }
		QMenu::item:selected { background: rgba(169,174,182,0.4); }
		/* Popover cards embedded IN the window (settings):
		   SOLID raised fill. The palette Window role carries
		   the macOS background-opacity alpha (frosted window), so
		   palette(window) fills went see-through — and plain-QWidget
		   palette fills are ignored entirely under an app stylesheet.
		   Translucent popup WINDOWS (finder, line jump, uri options)
		   style themselves locally instead — they don't resolve
		   app-sheet rules. */
		#settingsPopup {
			background: %2;
			border: 1px solid rgba(128, 128, 128, 110);
			border-radius: 6px;
		}
		QLabel { color: %3; background: transparent; }
		QScrollBar:vertical, QScrollBar:horizontal { width: 0; height: 0; }
		/* Editor horizontal scrollbar (ImGui HorizontalScrollbar parity).
		   The rule above zeroes EVERY scrollbar in the app (the editor's
		   vertical bar is replaced by the minimap); the id selector outranks
		   it, so only this named bar gets a real thin overlay look. */
		#nedHScroll { background: transparent; height: 10px; margin: 0; }
		#nedHScroll::handle:horizontal {
			background: rgba(255,255,255,0.28); border-radius: 4px; min-width: 28px;
			margin: 2px 1px;
		}
		#nedHScroll::handle:horizontal:hover { background: rgba(255,255,255,0.45); }
		#nedHScroll::handle:horizontal:pressed { background: rgba(255,255,255,0.6); }
		#nedHScroll::add-line:horizontal, #nedHScroll::sub-line:horizontal { width: 0; height: 0; }
		#nedHScroll::add-page:horizontal, #nedHScroll::sub-page:horizontal { background: transparent; }
		/* Editor vertical overlay bar (used when the minimap is off).
		   Without an id rule the global zeroing above blanks its 14px
		   strip entirely — the "empty scrollbar gap" on the right edge. */
		#nedVScroll { background: transparent; width: 10px; margin: 0; }
		#nedVScroll::handle:vertical {
			background: rgba(255,255,255,0.28); border-radius: 4px; min-height: 28px;
			margin: 1px 2px;
		}
		#nedVScroll::handle:vertical:hover { background: rgba(255,255,255,0.45); }
		#nedVScroll::handle:vertical:pressed { background: rgba(255,255,255,0.6); }
		#nedVScroll::add-line:vertical, #nedVScroll::sub-line:vertical { width: 0; height: 0; }
		#nedVScroll::add-page:vertical, #nedVScroll::sub-page:vertical { background: transparent; }
		/* Split handles and the dock separator are NOT styled here:
		   stylesheet backgrounds never render on them in the translucent
		   macOS window, which is why the 1px handles vanished. They are
		   painted in C++ — NedSplitterHandle (workbench) and the
		   PM_DockWidgetSeparatorExtent override in NedChromeStyle. */
		/* Bottom status bar (VSCode-style). The strip fill + hairline are
		   painted in NedStatusBar::paintEvent (QSS backgrounds never
		   render on a plain QWidget in the translucent macOS window). */
		#NedStatusBar QLabel { color: #a9aeb6; padding: 0 8px; }
		#NedStatusBar QLabel[tight="true"] { padding: 0 3px; }
		QDockWidget { titlebar-close-icon: none; titlebar-normal-icon: none; }
	)")
		// NOTE: the template no longer uses %1 — pass only %2..%5 in order,
		// or QString::arg shifts every color by one (the dark-on-dark bug).
		// The global background tint is NOT here: translucent windows skip
		// Qt's background machinery, so it is an explicit fill in
		// AppHost::paintEvent instead.
		.arg(raised, ink, ink, accent)
		.arg(fontPt)
		// %7..%13: font-scaled tab pill metrics (see z above).
		.arg(tabPadV)
		.arg(tabPadH)
		.arg(tabH)
		.arg(tabMTop)
		.arg(tabMSide)
		.arg(tabMBot)
		.arg(tabR)
		// %14: terminal font size (see the #terminalTabs rule above).
		.arg(monoPt);
}

} // namespace NedQtTheme
