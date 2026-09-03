/*
	File: views/qt/qt_row_text.h
	Description: Tab-expanded monospace row model — the ONE coordinate
	space the Qt editor paints, hit-tests and places carets in. Extracted
	from qt_editor_view so the model is testable apart from the widget.

	Visual indexes count CELLS: a tab occupies its expanded cells (one
	entry per cell), every other glyph exactly one — including
	astral-plane characters, which a QString alone would count as two
	UTF-16 units. `cellUnit` maps each cell to its code-unit span inside
	`expanded`, so drawing a cell never splits a surrogate pair.
*/

#pragma once

#include <QString>

#include <string>
#include <vector>

struct QtRowText
{
	QString expanded;			   // tab-expanded text (UTF-16 code units)
	std::vector<int> byteToVisual; // indexed by byte offset
	std::vector<int> visualToByte; // indexed by visual cell
	std::vector<int> cellUnit;	   // cell -> code units before it (size = cells + 1)

	int visualCount() const { return static_cast<int>(visualToByte.size()) - 1; }

	// Text of one visual cell (" " for tab filler). Allocates; call per
	// drawn cell only.
	QString cellText(int visual) const
	{
		return expanded.mid(cellUnit[static_cast<size_t>(visual)],
							cellUnit[static_cast<size_t>(visual) + 1] -
								cellUnit[static_cast<size_t>(visual)]);
	}
};

// Expand one document line. segmentStart rebases tab stops at a
// wrap-segment edge (the shared wrap layout measures every segment from
// x=0, so continuation rows restart their stops there); 0 = whole line.
QtRowText expandRowText(const std::string &line, int segmentStart = 0);
