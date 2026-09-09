/*
	File: host/qt/fonts.h
	Description: Profile font names are bundled file stems ("SourceCodePro-
	Regular"); Qt needs family names. This maps stems to the families
	registered from resources/fonts (host registers them at startup).
	Definitions in fonts.cpp.
*/

#pragma once

#include <QHash>
#include <QString>

class QFont;
class Settings;

namespace NedQtFonts {

QHash<QString, QString> &stemToFamily();
QHash<QString, QString> &familyToStem();

// Call once after addApplicationFont registrations: remembers
// file-stem -> real family (and back — the settings dialog persists
// STEMS: the ImGui host loads fonts by "<stem>.ttf").
void registerFontFile(const QString &filePath, int appFontId);

// Register every bundled font in resources/fonts — the ONE registration
// site (the Qt host's startup). Later callers (settings dialog) only
// READ the registered families; re-registering would duplicate them.
void registerBundledFonts();

// Resolve a profile font setting to a real Qt family (empty when unknown).
QString resolveFamily(const QString &setting);

// Reverse: the profile file stem for a Qt family (empty when unknown) —
// what gets written back to the profile so the ImGui host (which loads
// "<stem>.ttf") still finds the font.
QString resolveStem(const QString &family);

// The monospace editor/terminal font from the profile — the single
// resolution rule (profile family when registered, Menlo/Consolas
// fallback, always fixed-pitch). Editor view and terminal panel share
// it; qtermwidget derives its cell width from the font, so a
// proportional family would draw glyphs over each other.
QFont profileMonoFont(const Settings &settings);

// Terminal panel font: profileMonoFont + integer metrics. qtermwidget
// measures cell width with QFontMetrics (rounded-to-integer advances)
// but paints with sub-pixel advances — glyphs then render wider than
// the computed cells and the cursor drifts LEFT of the typed glyphs.
// Kerning off + ForceIntegerMetrics makes painted advance match the
// measured one (classic terminal cell fix), without touching the
// qtermwidget submodule.
QFont terminalFont(const Settings &settings);

} // namespace NedQtFonts
