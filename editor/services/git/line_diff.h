/*
	Line-level diff of two line arrays (no full-file join).
	Reports which lines in `neu` are LCS-insertions (1-based), plus counts.

	Used only for the in-editor gutter / +/- counts. Shell `git diff` uses the
	byte stream and is independent of this path.
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

// LCS on line arrays (prefix/suffix strip, then middle). Caller should pass a
// cached line vector updated incrementally — not a full linesInto every edit.
LineDiff diffLines(const std::vector<std::string> &oldLines,
				   const std::vector<std::string> &newLines);
