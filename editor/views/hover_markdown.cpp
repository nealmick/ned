#include "hover_markdown.h"

#include <algorithm>
#include <cctype>
#include <string_view>

std::vector<std::string> SplitHoverLines(std::string_view s)
{
	std::vector<std::string> lines;
	while (true)
	{
		const size_t nl = s.find('\n');
		std::string_view line = s.substr(0, nl == std::string_view::npos ? s.size() : nl);
		if (!line.empty() && line.back() == '\r')
			line.remove_suffix(1);
		lines.emplace_back(line);
		if (nl == std::string_view::npos)
			break;
		s = s.substr(nl + 1);
	}
	return lines;
}

std::vector<HoverMdBlock> ParseHoverMarkdown(const std::string &src)
{
	auto isFence = [](std::string_view line, std::string *langOut) {
		size_t i = 0;
		while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
			++i;
		if (i + 3 > line.size() || line[i] != '`' || line[i + 1] != '`' ||
			line[i + 2] != '`')
			return false;
		i += 3;
		if (langOut)
		{
			while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
				++i;
			const size_t start = i;
			while (i < line.size() &&
				   !std::isspace(static_cast<unsigned char>(line[i])) && line[i] != '`')
				++i;
			std::string lang(line.substr(start, i - start));
			for (char &c : lang)
				c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			*langOut = std::move(lang);
		}
		return true;
	};

	std::vector<HoverMdBlock> blocks;
	HoverMdBlock cur;
	bool inFence = false;

	for (const std::string &lineStr : SplitHoverLines(src))
	{
		const std::string_view line = lineStr;
		std::string lang;
		if (isFence(line, &lang))
		{
			if (!inFence)
			{
				if (!cur.text.empty() || cur.code)
					blocks.push_back(std::move(cur));
				cur = {};
				cur.code = true;
				cur.language = lang;
				inFence = true;
			} else
			{
				blocks.push_back(std::move(cur));
				cur = {};
				inFence = false;
			}
		} else if (inFence)
		{
			if (!cur.text.empty())
				cur.text += '\n';
			cur.text.append(line.data(), line.size());
		} else if (line.size() >= 3 &&
				   (line.find_first_not_of('-') == std::string_view::npos ||
					line.find_first_not_of('*') == std::string_view::npos))
		{
			if (!cur.text.empty())
			{
				blocks.push_back(std::move(cur));
				cur = {};
			}
			HoverMdBlock rule;
			rule.text = "---";
			blocks.push_back(std::move(rule));
		} else
		{
			if (!cur.text.empty())
				cur.text += '\n';
			cur.text.append(line.data(), line.size());
		}
	}

	if (!cur.text.empty() || cur.code)
		blocks.push_back(std::move(cur));

	// LSP servers pad fences with blank lines; the renderer separates blocks,
	// so keeping them would double the gap. Blank lines inside prose stay.
	for (auto &block : blocks)
	{
		if (block.code || block.text == "---")
			continue;
		const size_t first = block.text.find_first_not_of(" \t\r\n");
		if (first == std::string::npos)
		{
			block.text.clear();
			continue;
		}
		const size_t last = block.text.find_last_not_of(" \t\r\n");
		block.text = block.text.substr(first, last - first + 1);
	}
	blocks.erase(std::remove_if(blocks.begin(),
								blocks.end(),
								[](const HoverMdBlock &block) {
									return !block.code && block.text.empty();
								}),
				 blocks.end());
	return blocks;
}
