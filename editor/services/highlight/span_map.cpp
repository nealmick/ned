#include "span_map.h"
#include "../../util/editor_utils.h"

#include <algorithm>

void SpanMap::clear()
{
	lines.clear();
	lens.clear();
}

void SpanMap::assignEmpty(size_t lineCount)
{
	lines.assign(lineCount, {});
	lens.assign(lineCount, 0);
}

const LineColorSpans &SpanMap::at(int row) const
{
	static const LineColorSpans kEmpty;
	if (row < 0 || row >= static_cast<int>(lines.size()))
		return kEmpty;
	return lines[static_cast<size_t>(row)];
}

void SpanMap::insertBytes(LineColorSpans &spans, int col, int n)
{
	if (n <= 0)
		return;
	// Half-open: a span ending at `col` owns the insert.
	bool covered = false;
	for (ColorSpan &s : spans)
	{
		if (s.start >= col)
		{
			s.start += n;
			s.end += n;
		} else if (s.end >= col)
		{
			s.end += n;
			covered = true;
		}
	}
	if (covered)
		return;
	for (auto it = spans.rbegin(); it != spans.rend(); ++it)
	{
		if (it->end <= col)
		{
			it->end = col + n;
			return;
		}
	}
}

void SpanMap::deleteBytes(LineColorSpans &spans, int col, int n)
{
	if (n <= 0)
		return;
	const int delEnd = col + n;
	LineColorSpans out;
	out.reserve(spans.size());
	for (const ColorSpan &s : spans)
	{
		if (s.end <= col)
			out.push_back(s);
		else if (s.start >= delEnd)
			out.push_back({s.start - n, s.end - n, s.slot});
		else
		{
			if (s.start < col)
				out.push_back({s.start, col, s.slot});
			if (s.end > delEnd)
				out.push_back({col, s.end - n, s.slot});
		}
	}
	out.erase(
		std::remove_if(
			out.begin(), out.end(), [](const ColorSpan &s) { return s.start >= s.end; }),
		out.end());
	spans = std::move(out);
}

void SpanMap::insert(int row, int col, const std::string &text, const std::string &eol)
{
	if (text.empty() || row < 0)
		return;
	if (static_cast<size_t>(row) >= lines.size())
	{
		lines.resize(static_cast<size_t>(row) + 1);
		lens.resize(static_cast<size_t>(row) + 1, 0);
	}

	const auto parts = EditorUtils::SplitOnSeparator(text, eol);
	if (parts.size() == 1)
	{
		const int n = static_cast<int>(parts[0].size());
		insertBytes(lines[static_cast<size_t>(row)], col, n);
		lens[static_cast<size_t>(row)] += n;
		return;
	}

	LineColorSpans &cur = lines[static_cast<size_t>(row)];
	LineColorSpans head, tail;
	for (const ColorSpan &s : cur)
	{
		if (s.end <= col)
			head.push_back(s);
		else if (s.start >= col)
			tail.push_back({s.start - col, s.end - col, s.slot});
		else
		{
			head.push_back({s.start, col, s.slot});
			tail.push_back({0, s.end - col, s.slot});
		}
	}

	const int firstLen = static_cast<int>(parts[0].size());
	const int lastLen = static_cast<int>(parts.back().size());
	const int oldLen = lens[static_cast<size_t>(row)];
	const int tailLen = std::max(0, oldLen - col);

	insertBytes(head, col, firstLen);
	cur = std::move(head);
	lens[static_cast<size_t>(row)] = col + firstLen;

	std::vector<LineColorSpans> added(parts.size() - 1);
	std::vector<int> addedLens(parts.size() - 1);
	for (size_t i = 0; i + 1 < parts.size(); ++i)
		addedLens[i] = static_cast<int>(parts[i + 1].size());
	addedLens.back() = lastLen + tailLen;
	for (ColorSpan &s : tail)
	{
		s.start += lastLen;
		s.end += lastLen;
	}
	added.back() = std::move(tail);

	lines.insert(lines.begin() + row + 1, added.begin(), added.end());
	lens.insert(lens.begin() + row + 1, addedLens.begin(), addedLens.end());
}

void SpanMap::remove(int row, int col, int length, int sepLen)
{
	if (length <= 0 || lines.empty())
		return;

	row = std::clamp(row, 0, static_cast<int>(lines.size()) - 1);
	col = std::clamp(col, 0, lens[static_cast<size_t>(row)]);
	int remaining = length;
	sepLen = std::max(sepLen, 0);

	while (remaining > 0 && row < static_cast<int>(lines.size()))
	{
		const int lineLen = lens[static_cast<size_t>(row)];
		const int avail = lineLen - col;

		if (remaining <= avail)
		{
			deleteBytes(lines[static_cast<size_t>(row)], col, remaining);
			lens[static_cast<size_t>(row)] -= remaining;
			return;
		}

		deleteBytes(lines[static_cast<size_t>(row)], col, 1 << 30);
		remaining -= avail;
		lens[static_cast<size_t>(row)] = col;

		if (row + 1 >= static_cast<int>(lines.size()))
			return;

		if (remaining < sepLen)
			remaining = 0;
		else
			remaining -= sepLen;

		LineColorSpans next = std::move(lines[static_cast<size_t>(row + 1)]);
		const int nextLen = lens[static_cast<size_t>(row + 1)];
		for (ColorSpan &s : next)
		{
			s.start += col;
			s.end += col;
		}
		auto &dest = lines[static_cast<size_t>(row)];
		dest.insert(dest.end(), next.begin(), next.end());
		lens[static_cast<size_t>(row)] = col + nextLen;

		lines.erase(lines.begin() + row + 1);
		lens.erase(lens.begin() + row + 1);
	}
}

bool SpanMap::applyEdits(const std::vector<PendingTreeEdit> &edits, const std::string &eol)
{
	if (edits.empty())
		return false;
	if (lens.size() != lines.size() || lines.empty())
		return false;
	const int sepLen = static_cast<int>(eol.size());
	for (const PendingTreeEdit &pe : edits)
	{
		if (pe.op.kind == OpKind::Insert)
			insert(pe.op.row, pe.op.column, pe.op.text, eol);
		else
			remove(pe.op.row, pe.op.column, pe.op.length, sepLen);
	}
	return true;
}
