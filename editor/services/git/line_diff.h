/*
	Line-level diff of two line arrays (no full-file join).

	`diffLines` reports which lines in `neu` are LCS-insertions (1-based),
	plus counts — used by the in-editor gutter / +/- counts. Shell `git diff`
	uses the byte stream and is independent of this path.

	`alignLines` is the full alignment (ordered keep/delete/add ops with
	1-based old/new line numbers) — the diff VIEW builds its unified buffer
	from it. diffLines is derived from the same backtrack so both consumers
	always agree.
*/

#pragma once

#include <string>
#include <unordered_set>
#include <vector>

struct LineDiff
{
	std::unordered_set<int> addedLines; // 1-based indices into `neu`
	int additions = 0;
	int deletions = 0;
};

struct DiffOp
{
	enum class Kind {
		Keep,
		Delete, // line present only in `old` (newLine = 0)
		Add		// line present only in `new` (oldLine = 0)
	};
	Kind kind = Kind::Keep;
	int oldLine = 0; // 1-based into oldLines (0 on Add)
	int newLine = 0; // 1-based into newLines (0 on Delete)
};

// LCS on line arrays (prefix/suffix strip, then middle). Caller should pass a
// cached line vector updated incrementally — not a full linesInto every edit.
LineDiff diffLines(const std::vector<std::string> &oldLines,
				   const std::vector<std::string> &newLines);

// Ordered alignment old→new (top to bottom), same LCS as diffLines.
std::vector<DiffOp> alignLines(const std::vector<std::string> &oldLines,
							   const std::vector<std::string> &newLines);
