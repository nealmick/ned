// Per-line ThemeSlot spans + line lengths for insert/delete morph.
#pragma once

#include "../../editor_operations.h"
#include "tree_sitter.h"

#include <string>
#include <vector>

struct SpanMap
{
	ColorRangeMap lines;
	std::vector<int> lens;

	void clear();
	void assignEmpty(size_t lineCount);
	const LineColorSpans &at(int row) const;

	// Geometry only. Caller keeps lens in sync with the pre-edit document.
	void insert(int row, int col, const std::string &text, const std::string &eol);
	void remove(int row, int col, int length, int sepLen);

	// False if maps are empty/desynced (caller should rebuild from content).
	bool applyEdits(const std::vector<PendingTreeEdit> &edits, const std::string &eol);

	static void insertBytes(LineColorSpans &spans, int col, int n);
	static void deleteBytes(LineColorSpans &spans, int col, int n);
};
