#include "line_diff.h"

#include <algorithm>
#include <vector>

namespace {

// LCS on a[a0,a1) vs b[b0,b1). Emits 1-based indices into full `b` for additions.
LineDiff diffSlice(const std::vector<std::string> &a,
				   int a0,
				   int a1,
				   const std::vector<std::string> &b,
				   int b0,
				   int b1)
{
	LineDiff out;
	const int n = a1 - a0;
	const int m = b1 - b0;
	if (n <= 0 && m <= 0)
		return out;
	if (n <= 0)
	{
		out.additions = m;
		for (int j = 0; j < m; ++j)
			out.addedLines.insert(b0 + j + 1);
		return out;
	}
	if (m <= 0)
	{
		out.deletions = n;
		return out;
	}

	constexpr long kMaxCells = 4'000'000;
	if (static_cast<long>(n) * static_cast<long>(m) > kMaxCells)
	{
		// Middle still huge (e.g. massive replace). Mark only this slice of `b`
		// as added — never fall back to full-file index alignment (that marks
		// every line after a single insert as dirty).
		out.additions = m;
		out.deletions = n;
		for (int j = 0; j < m; ++j)
			out.addedLines.insert(b0 + j + 1);
		return out;
	}

	const int cols = m + 1;
	std::vector<int> dp(static_cast<size_t>(n + 1) * static_cast<size_t>(cols), 0);
	auto at = [&](int i, int j) -> int & {
		return dp[static_cast<size_t>(i) * static_cast<size_t>(cols) +
				  static_cast<size_t>(j)];
	};

	for (int i = 0; i < n; ++i)
	{
		for (int j = 0; j < m; ++j)
		{
			if (a[static_cast<size_t>(a0 + i)] == b[static_cast<size_t>(b0 + j)])
				at(i + 1, j + 1) = at(i, j) + 1;
			else
				at(i + 1, j + 1) = std::max(at(i + 1, j), at(i, j + 1));
		}
	}

	int i = n;
	int j = m;
	while (i > 0 && j > 0)
	{
		if (a[static_cast<size_t>(a0 + i - 1)] == b[static_cast<size_t>(b0 + j - 1)])
		{
			--i;
			--j;
		} else if (at(i, j - 1) >= at(i - 1, j))
		{
			// insertion of b[b0 + j - 1] → 1-based index in full document
			out.addedLines.insert(b0 + j);
			out.additions++;
			--j;
		} else
		{
			out.deletions++;
			--i;
		}
	}
	while (j > 0)
	{
		out.addedLines.insert(b0 + j);
		out.additions++;
		--j;
	}
	out.deletions += i;
	return out;
}

} // namespace

// Line-array diff for the git gutter. Real `git diff` aligns the byte stream;
// this only needs which lines in `b` are not in the LCS (shown as edited).
//
// Always strip common prefix/suffix first so a single insert in a large file
// leaves a tiny middle for LCS — never O(n) false positives from index align.
LineDiff diffLines(const std::vector<std::string> &a, const std::vector<std::string> &b)
{
	LineDiff out;
	int n = static_cast<int>(a.size());
	int m = static_cast<int>(b.size());
	if (n == 0 && m == 0)
		return out;

	if (n == 0)
	{
		out.additions = m;
		for (int i = 1; i <= m; ++i)
			out.addedLines.insert(i);
		return out;
	}
	if (m == 0)
	{
		out.deletions = n;
		return out;
	}

	// Common prefix of equal lines.
	int pre = 0;
	const int preMax = std::min(n, m);
	while (pre < preMax && a[static_cast<size_t>(pre)] == b[static_cast<size_t>(pre)])
		++pre;

	// Common suffix of equal lines (do not overlap prefix).
	int as = n;
	int bs = m;
	while (as > pre && bs > pre &&
		   a[static_cast<size_t>(as - 1)] == b[static_cast<size_t>(bs - 1)])
	{
		--as;
		--bs;
	}

	// Unchanged prefix/suffix: only the middle can contribute adds/deletes.
	return diffSlice(a, pre, as, b, pre, bs);
}
