#include "hover_runs.h"

#include <algorithm>

std::vector<HoverRun> HoverProseRuns(std::string_view s)
{
	std::vector<HoverRun> runs;
	size_t i = 0;
	while (i < s.size())
	{
		if (s[i] == '`')
		{
			const size_t end = s.find('`', i + 1);
			if (end != std::string_view::npos)
			{
				runs.push_back({s.substr(i + 1, end - i - 1), HoverRunStyle::Code});
				i = end + 1;
				continue;
			}
		}
		if (i + 1 < s.size() && s[i] == '*' && s[i + 1] == '*')
		{
			const size_t end = s.find("**", i + 2);
			if (end != std::string_view::npos)
			{
				runs.push_back({s.substr(i + 2, end - i - 2), HoverRunStyle::Bold});
				i = end + 2;
				continue;
			}
		}
		if (s[i] == '[')
		{
			const size_t close = s.find(']', i + 1);
			if (close != std::string_view::npos && close + 1 < s.size() &&
				s[close + 1] == '(')
			{
				const size_t endParen = s.find(')', close + 2);
				if (endParen != std::string_view::npos)
				{
					runs.push_back({s.substr(i + 1, close - i - 1), HoverRunStyle::Link});
					i = endParen + 1;
					continue;
				}
			}
		}

		// Plain run up to the next inline marker.
		size_t next = s.size();
		for (size_t j = i + 1; j < s.size(); ++j)
		{
			if (s[j] == '`' || s[j] == '[' ||
				(j + 1 < s.size() && s[j] == '*' && s[j + 1] == '*'))
			{
				next = j;
				break;
			}
		}
		runs.push_back({s.substr(i, next - i), HoverRunStyle::Text});
		i = next;
	}
	return runs;
}

std::vector<HoverCodeRun> HoverCodeRuns(std::string_view line, const LineColorSpans &spans)
{
	std::vector<HoverCodeRun> runs;
	const int n = static_cast<int>(line.size());
	size_t spanIdx = 0;
	int i = 0;
	while (i < n)
	{
		while (spanIdx < spans.size() && spans[spanIdx].end <= i)
			++spanIdx;

		HoverCodeRun run;
		run.themed = false;
		int runEnd = n;
		if (spanIdx < spans.size() && spans[spanIdx].start <= i)
		{
			run.themed = true;
			run.slot = spans[spanIdx].slot;
			runEnd = std::min(n, spans[spanIdx].end);
		} else if (spanIdx < spans.size())
		{
			runEnd = std::min(n, spans[spanIdx].start);
		}

		run.text = line.substr(static_cast<size_t>(i), static_cast<size_t>(runEnd - i));
		runs.push_back(run);
		i = runEnd;
	}
	return runs;
}
