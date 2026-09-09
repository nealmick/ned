/*
	File: views/qt/row_text.h
	Description: Tab-expanded monospace row model — the ONE coordinate
	space the Qt editor paints, hit-tests and places carets in. Extracted
	from editor_frame so the model is testable apart from the widget.

	Visual indexes count CELLS: a tab occupies its expanded cells (one
	entry per cell), every other glyph exactly one — including
	astral-plane characters, which a QString alone would count as two
	UTF-16 units. `cellUnit` maps each cell to its code-unit span inside
	`expanded`, so drawing a cell never splits a surrogate pair.

	Only a WINDOW of the line is ever expanded — the viewport's columns
	(wrap off) or one wrap segment — so a multi-megabyte single-line file
	costs O(window), never O(line), per paint/hit-test. Values stay
	absolute (byteToVisual yields absolute visual columns) so callers keep
	their existing arithmetic; indexes are window-relative.
*/

#pragma once

#include <QString>

#include <algorithm>
#include <climits>
#include <string>
#include <vector>

struct RowText
{
	int base = 0;		// byte offset the window starts at
	int visualBase = 0; // absolute visual column at base (0 = segment-rebased)

	QString expanded;			   // tab-expanded window text (UTF-16 code units)
	std::vector<int> byteToVisual; // (byte - base) -> absolute visual column
	std::vector<int> visualToByte; // (visual - visualBase) -> byte
	std::vector<int> cellUnit; // relative cell -> code units before it (size = cells + 1)

	int windowEndByte() const { return base + static_cast<int>(byteToVisual.size()) - 1; }
	int visualCount() const { return static_cast<int>(visualToByte.size()) - 1; }

	// Absolute visual column of a byte, clamped to the window borders —
	// columns far off-screen resolve to the window edge, which is exactly
	// where their rects/carets clip anyway.
	int visAt(int byte) const
	{
		return byteToVisual[static_cast<size_t>(
			std::clamp(byte - base, 0, static_cast<int>(byteToVisual.size()) - 1))];
	}
	// Byte at an absolute visual column, clamped to the window.
	int byteAt(int visual) const
	{
		return visualToByte[static_cast<size_t>(std::clamp(
			visual - visualBase, 0, static_cast<int>(visualToByte.size()) - 1))];
	}

	// Text of one visual cell (" " for tab filler). Allocates; call per
	// drawn cell only. `relativeCell` counts from the window start.
	QString cellText(int relativeCell) const
	{
		return expanded.mid(cellUnit[static_cast<size_t>(relativeCell)],
							cellUnit[static_cast<size_t>(relativeCell) + 1] -
								cellUnit[static_cast<size_t>(relativeCell)]);
	}
};

// Bytes covering visual columns [v0, v1) plus the visual column the
// window starts at. The walk stops once v1 is crossed, so its cost is
// proportional to the scrolled-to column, not the line length.
struct ByteWindow
{
	int from = 0;
	int to = 0;
	int visualBase = 0;
};
ByteWindow windowForVisual(const std::string &line, int v0, int v1);

// Expand bytes [fromByte, toByte). visualBase continues tab stops from
// that absolute column (wrap segments pass 0 to rebase stops at their
// edge, matching how the shared wrap layout measures each segment).
RowText
expandRowText(const std::string &line, int fromByte, int toByte, int visualBase = 0);

// Tab-expanded cell count of bytes [0, upToByte) — allocation-free walk
// (longest-line scan and caret x need a total, not an expansion).
int countVisualCells(const std::string &line, int upToByte = INT_MAX);
