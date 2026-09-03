#include "qt_row_text.h"

#include "../../util/text_columns.h"

QtRowText expandRowText(const std::string &line, int segmentStart)
{
	QtRowText rt;
	rt.byteToVisual.assign(line.size() + 1, 0);

	const bool rebase = segmentStart > 0 && segmentStart < static_cast<int>(line.size());
	int visual = 0;
	for (size_t i = 0; i < line.size();)
	{
		if (rebase && static_cast<int>(i) == segmentStart)
			visual = 0;
		if (rebase && static_cast<int>(i) < segmentStart)
		{
			++i; // prefix bytes: byteToVisual stays 0 (never read in-segment)
			continue;
		}
		rt.byteToVisual[i] = visual;
		const unsigned char c = static_cast<unsigned char>(line[i]);
		if (c == '\t')
		{
			// Tab stops every kTabSize visual cells (matches the shared
			// wrap layout's measurement).
			const int next = (visual / EditorUtils::kTabSize + 1) * EditorUtils::kTabSize;
			for (; visual < next; ++visual)
			{
				rt.cellUnit.push_back(static_cast<int>(rt.expanded.size()));
				rt.expanded += QLatin1Char(' ');
				rt.visualToByte.push_back(static_cast<int>(i));
			}
			++i;
			continue;
		}
		int len = 1;
		while (i + len < line.size() &&
			   (static_cast<unsigned char>(line[i + len]) & 0xC0) == 0x80)
			++len;
		rt.cellUnit.push_back(static_cast<int>(rt.expanded.size()));
		rt.expanded += QString::fromUtf8(line.data() + i, len);
		++visual;
		rt.visualToByte.push_back(static_cast<int>(i));
		i += static_cast<size_t>(len);
	}
	rt.byteToVisual[line.size()] = visual;
	rt.cellUnit.push_back(static_cast<int>(rt.expanded.size()));
	rt.visualToByte.push_back(static_cast<int>(line.size()));
	return rt;
}
