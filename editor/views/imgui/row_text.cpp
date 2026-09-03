#include "row_text.h"

#include "../../util/editor_utils.h"

namespace {
constexpr int TAB_SIZE = 4;
}

namespace RowText {
size_t advanceUtf8(const std::string &text, size_t index, size_t end)
{
	if (index >= end)
		return end;
	++index;
	while (index < end && (static_cast<unsigned char>(text[index]) & 0xC0) == 0x80)
		++index;
	return index;
}

float measureGlyphWidth(const char *start,
						const char *end,
						float draw_x,
						float text_origin_x)
{
	return EditorUtils::MeasureGlyphWidth(start, end, draw_x, text_origin_x, TAB_SIZE);
}
} // namespace RowText
