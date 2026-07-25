#include "editor_state.h"

#include <algorithm>
#include <filesystem>

EditorState::EditorState() : lineEnding(platformLineEnding()) {}

std::string EditorState::platformLineEnding()
{
#ifdef _WIN32
	return "\r\n";
#else
	return "\n";
#endif
}

std::pair<std::vector<std::string>, std::string>
EditorState::splitLines(const std::string &raw)
{
	std::vector<std::string> lines;
	bool sawCrLf = false;
	bool sawLf = false;
	bool sawCr = false;

	size_t start = 0;
	for (size_t i = 0; i < raw.size();)
	{
		if (raw[i] == '\r' && i + 1 < raw.size() && raw[i + 1] == '\n')
		{
			lines.emplace_back(raw, start, i - start);
			sawCrLf = true;
			i += 2;
			start = i;
		} else if (raw[i] == '\n')
		{
			lines.emplace_back(raw, start, i - start);
			sawLf = true;
			++i;
			start = i;
		} else if (raw[i] == '\r')
		{
			lines.emplace_back(raw, start, i - start);
			sawCr = true;
			++i;
			start = i;
		} else
		{
			++i;
		}
	}
	lines.emplace_back(raw, start, raw.size() - start);
	if (lines.empty())
		lines.emplace_back("");

	std::string ending;
	if (sawCrLf)
		ending = "\r\n";
	else if (sawLf)
		ending = "\n";
	else if (sawCr)
		ending = "\r";
	else
		ending = platformLineEnding();

	return {std::move(lines), std::move(ending)};
}

std::string EditorState::languageIdFromPath(const std::string &filePath)
{
	if (filePath.empty())
		return {};
	std::string ext = std::filesystem::path(filePath).extension().string();
	if (!ext.empty() && ext[0] == '.')
		ext.erase(0, 1);
	return ext;
}

static std::string joinLines(const std::vector<std::string> &lines, const std::string &le)
{
	if (lines.empty())
		return {};
	size_t total = lines[0].size();
	for (size_t i = 1; i < lines.size(); ++i)
		total += le.size() + lines[i].size();
	std::string out;
	out.reserve(total);
	out.append(lines[0]);
	for (size_t i = 1; i < lines.size(); ++i)
	{
		out.append(le);
		out.append(lines[i]);
	}
	return out;
}

void EditorState::setFromString(const std::string &raw)
{
	utf8Bom = raw.size() >= 3 && static_cast<unsigned char>(raw[0]) == 0xEF &&
			  static_cast<unsigned char>(raw[1]) == 0xBB &&
			  static_cast<unsigned char>(raw[2]) == 0xBF;

	auto [lines, ending] = utf8Bom ? splitLines(raw.substr(3)) : splitLines(raw);
	lineEnding = std::move(ending);
	text.assign(joinLines(lines, lineEnding));
	version = 0;
	dirty = false;
}

void EditorState::setLineEnding(const std::string &eol)
{
	if (eol.empty() || eol == lineEnding)
		return;
	std::vector<std::string> ls;
	linesInto(ls);
	lineEnding = eol;
	text.assign(joinLines(ls, lineEnding));
}

std::string EditorState::join() const { return text.str(); }

std::vector<std::string> EditorState::lines() const
{
	std::vector<std::string> out;
	linesInto(out);
	return out;
}

void EditorState::linesInto(std::vector<std::string> &out) const
{
	const int n = lineCount();
	if (static_cast<int>(out.size()) != n)
		out.resize(static_cast<size_t>(n));
	for (int i = 0; i < n; ++i)
		lineInto(i, out[static_cast<size_t>(i)]);
}

size_t EditorState::offsetFromRowCol(int row, int column) const
{
	return text.offsetFromRowCol(row, column);
}

void EditorState::rowColFromOffset(size_t offset, int &row, int &column) const
{
	text.rowColFromOffset(offset, row, column);
}

void EditorState::markEdited()
{
	++version;
	dirty = true;
}

void EditorState::markSaved() { dirty = false; }
