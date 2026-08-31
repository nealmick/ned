/*
	File: views/wrap_layout.h
	Description: Soft-wrap segment cache. Maps document rows → visual segments
	(byte start column per segment) plus a cumulative visual-line index.
	Owned by EditorFrame; consumers reach it via ViewLayout.wrap (null = off).
	Incremental like the frame's longest-line cache: noteEdit() marks a dirty
	row span, ensure() re-wraps only that span (full rebuild on width / font /
	version / line-count change).
*/

#pragma once

#include <string>
#include <vector>

class EditorState;

class WrapLayout
{
  public:
	struct Hit
	{
		int row = 0;
		int segment = 0; // 0-based segment within the row
	};

	// Full rebuild next ensure().
	void invalidate() { valid = false; }
	// Dirty row span after an edit (same call site as the width cache).
	void noteEdit(int lo, int hi);

	// Rebuild / refresh so accessors match the document. Cheap when clean.
	void ensure(const EditorState &state, float wrapWidth);

	int lineCount() const { return lines; }
	int totalVisualLines() const
	{
		return lines > 0 ? rowStart[static_cast<size_t>(lines)] : 0;
	}
	// Visual lines occupied by one row (1 when it does not wrap).
	int visualLineCount(int row) const;
	// First visual line index of a row (row * 1 when not wrapping).
	int rowStartVisualLine(int row) const;
	// Inverse: visual line index (y / lineHeight) → row + segment.
	Hit yToRow(float visualLine) const;
	// Which segment a byte column falls in.
	int segmentOf(int row, int column) const;
	int segmentCount(int row) const;
	// Byte column where a segment starts (0 for segment 0).
	// Out-of-range segment (> segmentCount-1) returns INT_MAX — callers use it
	// as a "no more segments" sentinel (never dereferenced as a column).
	int segmentStartColumn(int row, int segment) const;

	// Pixel helpers. x is relative to the segment's left edge (add textPos.x).
	float columnX(const std::string &line, int row, int column) const;
	int columnAt(const std::string &line, int row, int segment, float xRel) const;

  private:
	bool valid = false;
	float width = -1.0f;   // wrap width the cache was built with
	float fontKey = -1.0f; // font size at build time
	int lines = -1;
	int dirtyLo = 0, dirtyHi = -1;

	// breaks[row]: start columns of segments 1..n (segment 0 always starts at 0).
	std::vector<std::vector<int>> breaks;
	// Cumulative visual lines before each row (size lines + 1).
	std::vector<int> rowStart;

	void wrapRow(const std::string &line, float limit, std::vector<int> &out) const;
	void rebuildCumulative();
};
