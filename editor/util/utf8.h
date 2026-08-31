#pragma once

#include <string_view>

namespace EditorUtils {

// Decode one UTF-8 codepoint starting at `i`. Returns bytes consumed (at least 1
// unless `i` is at end). Invalid sequences consume 1 byte and yield U+FFFD.
inline int DecodeUtf8(std::string_view s, int i, unsigned int &cp)
{
	const int n = static_cast<int>(s.size());
	if (i < 0 || i >= n)
	{
		cp = 0;
		return 0;
	}

	const unsigned char c0 = static_cast<unsigned char>(s[static_cast<size_t>(i)]);
	if (c0 < 0x80)
	{
		cp = c0;
		return 1;
	}
	if ((c0 & 0xE0) == 0xC0 && i + 1 < n)
	{
		const unsigned char c1 =
			static_cast<unsigned char>(s[static_cast<size_t>(i + 1)]);
		if ((c1 & 0xC0) == 0x80)
		{
			cp = (static_cast<unsigned int>(c0 & 0x1F) << 6) | (c1 & 0x3F);
			if (cp >= 0x80)
				return 2;
		}
	} else if ((c0 & 0xF0) == 0xE0 && i + 2 < n)
	{
		const unsigned char c1 =
			static_cast<unsigned char>(s[static_cast<size_t>(i + 1)]);
		const unsigned char c2 =
			static_cast<unsigned char>(s[static_cast<size_t>(i + 2)]);
		if ((c1 & 0xC0) == 0x80 && (c2 & 0xC0) == 0x80)
		{
			cp = (static_cast<unsigned int>(c0 & 0x0F) << 12) |
				 (static_cast<unsigned int>(c1 & 0x3F) << 6) | (c2 & 0x3F);
			if (cp >= 0x800)
				return 3;
		}
	} else if ((c0 & 0xF8) == 0xF0 && i + 3 < n)
	{
		const unsigned char c1 =
			static_cast<unsigned char>(s[static_cast<size_t>(i + 1)]);
		const unsigned char c2 =
			static_cast<unsigned char>(s[static_cast<size_t>(i + 2)]);
		const unsigned char c3 =
			static_cast<unsigned char>(s[static_cast<size_t>(i + 3)]);
		if ((c1 & 0xC0) == 0x80 && (c2 & 0xC0) == 0x80 && (c3 & 0xC0) == 0x80)
		{
			cp = (static_cast<unsigned int>(c0 & 0x07) << 18) |
				 (static_cast<unsigned int>(c1 & 0x3F) << 12) |
				 (static_cast<unsigned int>(c2 & 0x3F) << 6) | (c3 & 0x3F);
			if (cp >= 0x10000 && cp <= 0x10FFFF)
				return 4;
		}
	}

	cp = 0xFFFD;
	return 1;
}

inline int Utf16UnitsForCodepoint(unsigned int cp) { return cp > 0xFFFF ? 2 : 1; }

// UTF-16 code units in `s[0, byteOffset)`. `byteOffset` is a UTF-8 byte index
// (clamped). Mid-sequence offsets snap back to the character start.
inline int Utf8ByteOffsetToUtf16(std::string_view s, int byteOffset)
{
	const int n = static_cast<int>(s.size());
	if (byteOffset <= 0)
		return 0;
	if (byteOffset > n)
		byteOffset = n;

	int units = 0;
	for (int i = 0; i < byteOffset;)
	{
		unsigned int cp = 0;
		const int adv = DecodeUtf8(s, i, cp);
		if (adv <= 0)
			break;
		if (i + adv > byteOffset)
			break;
		units += Utf16UnitsForCodepoint(cp);
		i += adv;
	}
	return units;
}

// Byte offset in `s` for `utf16Units` UTF-16 code units from the start.
inline int Utf16ToUtf8ByteOffset(std::string_view s, int utf16Units)
{
	const int n = static_cast<int>(s.size());
	if (utf16Units <= 0)
		return 0;

	int units = 0;
	int i = 0;
	while (i < n && units < utf16Units)
	{
		unsigned int cp = 0;
		const int adv = DecodeUtf8(s, i, cp);
		if (adv <= 0)
			break;
		units += Utf16UnitsForCodepoint(cp);
		i += adv;
	}
	return i;
}

} // namespace EditorUtils
