#pragma once

/*
	Completion model + matching helpers, shared by the LSP core, the backend
	views and the unit tests. Deliberately free of lsp-framework types: the
	core normalizes generated CompletionItems into LSPCompletionEntry once
	(on the reader thread) and everything downstream — filtering, popup
	rendering, tests — works on this plain shape only.

	Columns are UTF-8 byte columns, the same space EditorState/EditorCommands
	use; the LSP core converts to UTF-16 only at the protocol boundary.
*/

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

struct LSPCompletionEntry
{
	std::string label;
	std::string insertText; // what accepting types (falls back to label)
	std::string filterText; // empty -> label
	std::string sortText;	// empty -> label
	std::string detail;		// type/signature line, dimmed in the popup
	int kind = 0;			// lsp CompletionItemKind (0 = unknown)
	bool deprecated = false;
	bool preselect = false;
};

// Bytes that extend an identifier under the caret (ASCII word chars; any
// non-ASCII byte also counts so Unicode identifiers stay one word).
inline bool lspCompletionWordByte(char c)
{
	const unsigned char u = static_cast<unsigned char>(c);
	if (u >= 0x80)
		return true; // UTF-8 continuation/lead — treat as word
	return std::isalnum(u) != 0 || c == '_';
}

// Byte column where the word containing the caret's left side starts.
// `caretByte` may sit mid-character; the scan works on byte boundaries.
inline int lspCompletionWordStart(std::string_view line, int caretByte)
{
	const int end = std::clamp(caretByte, 0, static_cast<int>(line.size()));
	int start = end;
	while (start > 0 && lspCompletionWordByte(line[static_cast<size_t>(start - 1)]))
		--start;
	return start;
}

inline char lspCompletionLower(char c)
{
	return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

// Short display name for a completion label (VSCode-style): servers send
// scope-qualified labels ("ns::Class::method") — show only the last
// segment. Dots are deliberately NOT treated as scope separators: labels
// like "CMakeLists.txt" or "obj.member" must render whole. Filtering still
// runs against the full label and the server's insert text is untouched.
inline std::string_view lspCompletionShortLabel(std::string_view label)
{
	std::string_view shortLabel = label;
	const size_t scope = shortLabel.rfind("::");
	if (scope != std::string_view::npos && scope + 2 < shortLabel.size())
		shortLabel = shortLabel.substr(scope + 2);
	return shortLabel;
}

// Case-insensitive prefix / word-boundary / subsequence match. Returns false
// when `text` can't complete `prefix`; rank 0 is best (exact prefix).
inline bool
lspCompletionMatch(std::string_view text, std::string_view lowerPrefix, int &rank)
{
	if (lowerPrefix.empty())
	{
		rank = 0;
		return true;
	}
	if (text.size() < lowerPrefix.size())
		return false;

	// Rank 0: plain (case-insensitive) prefix.
	bool prefix = true;
	for (size_t i = 0; i < lowerPrefix.size(); ++i)
		if (lspCompletionLower(text[i]) != lowerPrefix[i])
		{
			prefix = false;
			break;
		}
	if (prefix)
	{
		rank = 0;
		return true;
	}

	// Rank 1: prefix of a word inside the text (camelCase, snake_case,
	// dot/scope separators) — "comp" matches "someCompletion".
	for (size_t i = 0; i + lowerPrefix.size() <= text.size(); ++i)
	{
		if (i > 0 && lspCompletionWordByte(text[i - 1]))
			continue; // not a boundary
		bool hit = true;
		for (size_t j = 0; j < lowerPrefix.size(); ++j)
			if (lspCompletionLower(text[i + j]) != lowerPrefix[j])
			{
				hit = false;
				break;
			}
		if (hit)
		{
			rank = 1;
			return true;
		}
	}

	// Rank 2: subsequence anywhere ("gcc" -> "getConfigColumn").
	size_t at = 0;
	for (char c : text)
	{
		if (at < lowerPrefix.size() && lspCompletionLower(c) == lowerPrefix[at])
			++at;
		if (at == lowerPrefix.size())
			break;
	}
	if (at == lowerPrefix.size())
	{
		rank = 2;
		return true;
	}
	return false;
}

// Filter `items` by `prefix` (case-insensitive) and return indices into
// `items`, best match first: rank, then sortText (server's tie-break),
// then label. Stable for equal keys.
inline std::vector<int>
lspFilterCompletionItems(const std::vector<LSPCompletionEntry> &items,
						 std::string_view prefix)
{
	std::string lowerPrefix;
	lowerPrefix.reserve(prefix.size());
	for (char c : prefix)
		lowerPrefix.push_back(lspCompletionLower(c));

	struct Ranked
	{
		int index = 0;
		int rank = 0;
	};
	std::vector<Ranked> hits;
	for (int i = 0; i < static_cast<int>(items.size()); ++i)
	{
		const LSPCompletionEntry &e = items[static_cast<size_t>(i)];
		// Match what the user SEES: the short (unqualified) label — matching
		// the full "ns::name" would surface items whose visible name looks
		// unrelated to the typed prefix. An explicit server filterText wins
		// (clangd's is the unqualified name too).
		const std::string_view text = e.filterText.empty()
										  ? lspCompletionShortLabel(e.label)
										  : std::string_view(e.filterText);
		Ranked r{i, 0};
		if (lspCompletionMatch(text, lowerPrefix, r.rank))
			hits.push_back(r);
	}

	std::sort(hits.begin(), hits.end(), [&items](const Ranked &a, const Ranked &b) {
		if (a.rank != b.rank)
			return a.rank < b.rank;
		const LSPCompletionEntry &ea = items[static_cast<size_t>(a.index)];
		const LSPCompletionEntry &eb = items[static_cast<size_t>(b.index)];
		const std::string_view sa = ea.sortText.empty() ? std::string_view(ea.label)
														: std::string_view(ea.sortText);
		const std::string_view sb = eb.sortText.empty() ? std::string_view(eb.label)
														: std::string_view(eb.sortText);
		if (sa != sb)
			return sa < sb;
		return lspCompletionShortLabel(ea.label) < lspCompletionShortLabel(eb.label);
	});

	std::vector<int> out;
	out.reserve(hits.size());
	for (const Ranked &r : hits)
		out.push_back(r.index);
	return out;
}
