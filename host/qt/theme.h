/*
	File: host/qt/theme.h
	Description: Builds a Qt application palette from the shared settings
	profile (background + theme text), so the Qt chrome (sidebar, welcome,
	title bars) matches the editor's theme like the ImGui build.
*/

#pragma once

#include "../../../util/settings.h"

#include <QApplication>
#include <QColor>
#include <QPalette>

#include <algorithm>

namespace NedQtTheme {

// Profile colors are stored as float arrays ([r, g, b] or [r, g, b, a],
// 0..1). One parser for every consumer (theme background, settings dialog
// swatch) so the fallback stays consistent everywhere.
QColor colorFromJson(const json *arr, const QColor &fallback);

// Background used when the profile has no backgroundColor key — the ONE
// definition (the settings dialog stages the same value into its swatch).
QColor defaultBackground();

// The theme background from the profile (backgroundColor array), with the
// bundled default when absent.
QColor background(const Settings &s);

// Active theme's text color (falls back to a light gray).
QColor text(const Settings &s);

// Slightly lighter than the background — title bars, separators.
QColor raised(const QColor &bg);

// Opaque popover tone from the LIVE app palette (no Settings needed) —
// same lifted recipe as raised(), alpha forced solid. Translucent popup
// WINDOWS (finder, line jump, LSP uri options) don't resolve
// app-stylesheet background rules, so they style themselves locally
// with this.
QString popoverColor();

// The ONE "nedPopoverCard" sheet. Popover pattern, used everywhere
// (finder, line jump, uri options, hover tip): a translucent Qt::Popup
// top level whose OPAQUE card child carries the styling — QSS
// backgrounds never render on the translucent top level itself, and a
// palette(window) fill would carry the macOS background-opacity alpha.
// The rule MUST stay scoped to the card (the objectName selector): a
// selector-less block applies to every descendant too, drawing the
// hairline border around the title, input and list (the "extra
// borders" bug).
QString popoverCardSheet(int radius = 11);

QPalette palette(const Settings &s);

// Rounded styling throughout; all tabs keep identical geometry (the
// active tab changes color, never size). monoPt = terminal font size —
// pinned inside #terminalTabs so the universal font-size rule can't
// desync the terminal's cell metrics from its rendered glyphs.
QString appStyleSheet(const Settings &s, int fontPt, int monoPt);

} // namespace NedQtTheme
