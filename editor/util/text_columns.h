/*
	File: util/text_columns.h
	Description: Pure text/UTF-8/word helpers over std::string — no UI
	toolkit. Shared by the editor core and every backend.
*/

#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace EditorUtils {

inline int SnapToUtf8CharBoundary(const std::string &str, int idx)
{
	if (idx <= 0 || idx >= (int)str.size())
		return idx;
	while (idx > 0 && (static_cast<unsigned char>(str[idx]) & 0xC0) == 0x80)
	{
		--idx;
	}
	return idx;
}

// Encode a Unicode codepoint as UTF-8 into `out`. Returns bytes written (0 if invalid).
inline int AppendUtf8Codepoint(std::string &out, unsigned int cp)
{
	if (cp < 0x80)
	{
		out.push_back(static_cast<char>(cp));
		return 1;
	}
	if (cp < 0x800)
	{
		out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
		out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		return 2;
	}
	if (cp < 0x10000)
	{
		out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
		out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
		out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		return 3;
	}
	if (cp <= 0x10FFFF)
	{
		out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
		out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
		out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
		out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		return 4;
	}
	return 0;
}

inline bool IsWordChar(char c)
{
	return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

// Word boundaries within a single line (byte offsets into `line`).
inline void FindWordBoundaries(const std::string &line, int pos, int &start, int &end)
{
	const int len = static_cast<int>(line.size());
	if (pos < 0)
		pos = 0;
	if (pos > len)
		pos = len;

	int probe = pos;
	if (probe >= len || !IsWordChar(line[probe]))
	{
		if (probe > 0 && IsWordChar(line[probe - 1]))
			--probe;
		else
		{
			start = end = pos;
			return;
		}
	}

	start = probe;
	while (start > 0 && IsWordChar(line[start - 1]))
		--start;

	end = probe + 1;
	while (end < len && IsWordChar(line[end]))
		++end;
}

inline void MoveToPrevUtf8Char(std::string::iterator &it)
{
	--it;
	while ((static_cast<unsigned char>(*it) & 0xC0) == 0x80)
	{
		--it;
	}
}

inline void MoveToNextUtf8Char(std::string::iterator &it)
{
	++it;
	while ((static_cast<unsigned char>(*it) & 0xC0) == 0x80)
	{
		++it;
	}
}

// Split `text` on a fixed separator (e.g. document line ending). Trailing
// separator yields a final empty part (same as morph / paste needs).
inline std::vector<std::string> SplitOnSeparator(const std::string &text,
												 const std::string &sep)
{
	if (sep.empty())
		return {text};
	std::vector<std::string> parts;
	size_t start = 0;
	while (start <= text.size())
	{
		const size_t pos = text.find(sep, start);
		if (pos == std::string::npos)
		{
			parts.emplace_back(text, start, text.size() - start);
			break;
		}
		parts.emplace_back(text, start, pos - start);
		start = pos + sep.size();
		if (start == text.size())
		{
			parts.emplace_back("");
			break;
		}
	}
	if (parts.empty())
		parts.emplace_back("");
	return parts;
}

// Monospace tab/glyph width (shared by text view, caret, hit-test).
inline constexpr int kTabSize = 4;

inline float TabAdvanceWidth(float spaceWidth, int visualColumn, int tabSize = kTabSize)
{
	const int nextTab = ((visualColumn / tabSize) + 1) * tabSize;
	return static_cast<float>(nextTab - visualColumn) * spaceWidth;
}


} // namespace EditorUtils
