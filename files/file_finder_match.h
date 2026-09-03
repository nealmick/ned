#pragma once

/*
	files/file_finder_match.h
	Backend-neutral scan rules + fuzzy matching for the Ctrl+P file
	finder — the ONE implementation shared by the ImGui and Qt finders so
	results (and the ignore list) can't drift between backends.
*/

#include "file_finder.h"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <vector>

namespace FileFinderMatch {

// Directories never scanned: VCS data, build outputs, heavy caches.
inline bool shouldSkipDir(std::string_view name)
{
	return name == ".git" || name == ".build" || name == ".build-qt" ||
		   name == ".build-min" || name == "build" || name == "node_modules" ||
		   name == "dist" || name == ".cache";
}

// Subsequence fuzzy score over LOWERCASED text: matched-in-order wins,
// consecutive matches and matches at the start / after a separator
// (/, _, -) score higher. Returns -1 when the query is not a
// subsequence of the candidate.
inline int fuzzyMatchScore(std::string_view candidate, std::string_view query)
{
	if (query.empty())
		return 0;
	int score = 0;
	int ci = 0;
	int prevHit = -2;
	for (size_t qi = 0; qi < query.size(); ++qi)
	{
		const char qc =
			static_cast<char>(std::tolower(static_cast<unsigned char>(query[qi])));
		int hit = -1;
		for (; ci < static_cast<int>(candidate.size()); ++ci)
		{
			if (std::tolower(
					static_cast<unsigned char>(candidate[static_cast<size_t>(ci)])) == qc)
			{
				hit = ci;
				break;
			}
		}
		if (hit < 0)
			return -1;
		// Consecutive matches and matches after a separator score better.
		score += (hit == prevHit + 1) ? 3 : 1;
		if (hit == 0 || candidate[static_cast<size_t>(hit) - 1] == '/' ||
			candidate[static_cast<size_t>(hit) - 1] == '_' ||
			candidate[static_cast<size_t>(hit) - 1] == '-')
			score += 2;
		prevHit = hit;
		ci = hit + 1;
	}
	return score;
}

// Filter + rank: dotfiles hidden unless the query itself contains a '.',
// best fuzzy score first, ties broken by the shorter path (the ImGui
// finder's old "shorter is usually the better match" rule), capped at
// maxResults.
inline std::vector<FileEntry> filterFiles(const std::vector<FileEntry> &files,
										  std::string_view queryLower,
										  size_t maxResults)
{
	std::vector<std::pair<int, const FileEntry *>> ranked;
	const bool queryHasDot = queryLower.find('.') != std::string_view::npos;
	for (const FileEntry &file : files)
	{
		const int score =
			FileFinderMatch::fuzzyMatchScore(file.relativePathLower, queryLower);
		if (score < 0)
			continue;
		// Hide dotfiles unless the query itself contains a '.'.
		if (!queryHasDot && !file.filenameLower.empty() && file.filenameLower[0] == '.')
			continue;
		ranked.emplace_back(score, &file);
	}
	std::stable_sort(ranked.begin(), ranked.end(), [](const auto &a, const auto &b) {
		if (a.first != b.first)
			return a.first > b.first;
		// Shorter paths win score ties (usually the better match).
		return a.second->relativePath.size() < b.second->relativePath.size();
	});
	std::vector<FileEntry> out;
	out.reserve(ranked.size());
	for (const auto &entry : ranked)
		out.push_back(*entry.second);
	if (out.size() > maxResults)
		out.resize(maxResults);
	return out;
}

} // namespace FileFinderMatch
