#include "line_diff.h"

#include <algorithm>
#include <vector>

namespace {

// LCS on a[a0,a1) vs b[b0,b1). Backtracks emitting ops (reversed), caller
// reverses. Ops carry 1-based indices into the FULL arrays.
void diffSlice(const std::vector<std::string> &a,
			   int a0,
			   int a1,
			   const std::vector<std::string> &b,
			   int b0,
			   int b1,
			   std::vector<DiffOp> &out)
{
	const int n = a1 - a0;
	const int m = b1 - b0;
	if (n <= 0 && m <= 0)
		return;
	if (n <= 0)
	{
		for (int j = 0; j < m; ++j)
			out.push_back(DiffOp{DiffOp::Kind::Add, 0, b0 + j + 1});
		return;
	}
	if (m <= 0)
	{
		for (int i = 0; i < n; ++i)
			out.push_back(DiffOp{DiffOp::Kind::Delete, a0 + i + 1, 0});
		return;
	}

	constexpr long kMaxCells = 4'000'000;
	if (static_cast<long>(n) * static_cast<long>(m) > kMaxCells)
	{
		// Middle still huge (e.g. massive replace). Mark the whole slice
		// replaced — never fall back to full-file index alignment (that
		// marks every line after a single insert as dirty).
		for (int i = 0; i < n; ++i)
			out.push_back(DiffOp{DiffOp::Kind::Delete, a0 + i + 1, 0});
		for (int j = 0; j < m; ++j)
			out.push_back(DiffOp{DiffOp::Kind::Add, 0, b0 + j + 1});
		return;
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

	// Backtrack (reversed order; same tie-break as the historical diffLines
	// so the gutter's marks don't shift).
	std::vector<DiffOp> rev;
	int i = n;
	int j = m;
	while (i > 0 && j > 0)
	{
		if (a[static_cast<size_t>(a0 + i - 1)] == b[static_cast<size_t>(b0 + j - 1)])
		{
			rev.push_back(DiffOp{DiffOp::Kind::Keep, a0 + i, b0 + j});
			--i;
			--j;
		} else if (at(i, j - 1) >= at(i - 1, j))
		{
			rev.push_back(DiffOp{DiffOp::Kind::Add, 0, b0 + j});
			--j;
		} else
		{
			rev.push_back(DiffOp{DiffOp::Kind::Delete, a0 + i, 0});
			--i;
		}
	}
	while (j > 0)
	{
		rev.push_back(DiffOp{DiffOp::Kind::Add, 0, b0 + j});
		--j;
	}
	while (i > 0)
	{
		rev.push_back(DiffOp{DiffOp::Kind::Delete, a0 + i, 0});
		--i;
	}
	out.insert(out.end(), rev.rbegin(), rev.rend());
}

std::vector<DiffOp> alignInternal(const std::vector<std::string> &a,
								  const std::vector<std::string> &b)
{
	std::vector<DiffOp> ops;
	const int n = static_cast<int>(a.size());
	const int m = static_cast<int>(b.size());

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

	for (int i = 0; i < pre; ++i)
		ops.push_back(DiffOp{DiffOp::Kind::Keep, i + 1, i + 1});
	diffSlice(a, pre, as, b, pre, bs, ops);
	for (int i = as; i < n; ++i)
		ops.push_back(DiffOp{DiffOp::Kind::Keep, i + 1, bs + (i - as) + 1});
	return ops;
}

} // namespace

std::vector<DiffOp> alignLines(const std::vector<std::string> &oldLines,
							   const std::vector<std::string> &newLines)
{
	return alignInternal(oldLines, newLines);
}

LineDiff diffLines(const std::vector<std::string> &oldLines,
				   const std::vector<std::string> &newLines)
{
	LineDiff out;
	for (const DiffOp &op : alignInternal(oldLines, newLines))
	{
		if (op.kind == DiffOp::Kind::Add)
		{
			out.addedLines.insert(op.newLine);
			out.additions++;
		} else if (op.kind == DiffOp::Kind::Delete)
			out.deletions++;
	}
	return out;
}
