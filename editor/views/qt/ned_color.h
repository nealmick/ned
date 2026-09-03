/*
	File: views/qt/ned_color.h
	Description: NedColor -> Qt color conversion. The only place the Qt
	backend translates core colors.
*/

#pragma once

#include "../../platform/ned_types.h"
#include <QColor>

inline QColor toQColor(const NedColor &c) { return QColor::fromRgbF(c.r, c.g, c.b, c.a); }
