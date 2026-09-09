#include "row_text.h"

#include "../../util/text_columns.h"

namespace {
// UTF-8 glyph span at `i` (lead byte + continuation bytes), bounded by `limit`.
size_t utf8SpanAt(const std::string &text, size_t i, size_t limit)
{
	size_t e = i + 1;
	while (e < limit && (static_cast<unsigned char>(text[e]) & 0xC0) == 0x80)
		++e;
	return e;
}
} // namespace

RowText expandRowText(const std::string &line, int fromByte, int toByte, int visualBase)
{
	RowText rt;
	const int len = static_cast<int>(line.size());
	fromByte = std::clamp(fromByte, 0, len);
	toByte = std::clamp(toByte, fromByte, len);
	rt.base = fromByte;
	rt.visualBase = visualBase;
	rt.byteToVisual.assign(static_cast<size_t>(toByte - fromByte + 1), visualBase);

	int visual = visualBase;
	size_t w = 0; // window-relative byte index (byte - base)
	for (int i = fromByte; i < toByte;)
	{
		rt.byteToVisual[w] = visual;
		const unsigned char c = static_cast<unsigned char>(line[static_cast<size_t>(i)]);
		if (c == '\t')
		{
			// Tab stops every kTabSize visual cells, continuing from
			// visualBase (the segment/viewport frame the caller measures in).
			const int next = (visual / EditorUtils::kTabSize + 1) * EditorUtils::kTabSize;
			for (; visual < next; ++visual)
			{
				rt.cellUnit.push_back(static_cast<int>(rt.expanded.size()));
				rt.expanded += QLatin1Char(' ');
				rt.visualToByte.push_back(i);
			}
			++i;
			++w;
			continue;
		}
		const size_t start = static_cast<size_t>(i);
		const size_t e = utf8SpanAt(line, start, static_cast<size_t>(toByte));
		rt.cellUnit.push_back(static_cast<int>(rt.expanded.size()));
		rt.expanded +=
			QString::fromUtf8(line.data() + start, static_cast<int>(e - start));
		++visual;
		rt.visualToByte.push_back(i);
		w += static_cast<int>(e - start);
		i = static_cast<int>(e);
	}
	rt.byteToVisual.back() = visual;
	rt.cellUnit.push_back(static_cast<int>(rt.expanded.size()));
	rt.visualToByte.push_back(toByte);
	return rt;
}

ByteWindow windowForVisual(const std::string &line, int v0, int v1)
{
	ByteWindow w{0, static_cast<int>(line.size()), 0};
	if (v1 <= 0)
	{
		w.to = 0;
		return w;
	}
	if (v0 < 0)
		v0 = 0;

	int visual = 0;
	bool started = v0 == 0;
	for (size_t i = 0; i < line.size();)
	{
		if (visual >= v1)
		{
			w.to = static_cast<int>(i);
			return w;
		}
		if (!started && visual >= v0)
		{
			w.from = static_cast<int>(i);
			w.visualBase = visual;
			started = true;
		}
		const unsigned char c = static_cast<unsigned char>(line[i]);
		if (c == '\t')
			visual = (visual / EditorUtils::kTabSize + 1) * EditorUtils::kTabSize;
		else
			++visual;
		i = utf8SpanAt(line, i, line.size());
	}
	if (!started)
	{
		// Viewport is entirely past the line's end: empty window.
		w.from = w.to = static_cast<int>(line.size());
		w.visualBase = visual;
	}
	return w;
}

int countVisualCells(const std::string &line, int upToByte)
{
	const size_t limit =
		std::min(line.size(), static_cast<size_t>(std::max(0, upToByte)));
	int visual = 0;
	for (size_t i = 0; i < limit;)
	{
		if (line[i] == '\t')
			visual = (visual / EditorUtils::kTabSize + 1) * EditorUtils::kTabSize;
		else
			++visual;
		i = utf8SpanAt(line, i, limit);
	}
	return visual;
}
