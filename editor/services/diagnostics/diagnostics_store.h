#pragma once

/*
	Workspace diagnostic store. Filled by the LSP client (publishDiagnostics),
	read by the editor gutter / text view. No lsp-framework types.
	Keyed by a normalized filesystem path.
*/

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

struct DiagnosticItem
{
	int startLine = 0;		// 0-based
	int startCharacter = 0; // UTF-16 (LSP)
	int endLine = 0;
	int endCharacter = 0;
	int severity = 1; // 1 error, 2 warning, 3 info, 4 hint
	std::string message;
	std::string source;
};

// Does `d` cover (line, utf16Column)? Works on an already-fetched item so
// callers holding a forLine() snapshot don't re-query the store.
inline bool DiagnosticContains(const DiagnosticItem &d, int line, int utf16Column)
{
	const bool afterStart =
		line > d.startLine || (line == d.startLine && utf16Column >= d.startCharacter);
	const bool beforeEnd =
		line < d.endLine || (line == d.endLine && utf16Column <= d.endCharacter);
	return afterStart && beforeEnd;
}

class LSPDiagnostics
{
  public:
	// Replace the diagnostic set for a document. `version` is the document
	// version the diagnostics were computed against (LSP publishDiagnostics),
	// or -1 when the server omits it. Stale (older) versions are ignored so a
	// slow server cannot overwrite fresher results after quick edits.
	void
	replace(const std::string &path, std::vector<DiagnosticItem> items, int version = -1);
	void clear(const std::string &path);
	void clearAll();

	std::vector<DiagnosticItem> forDocument(const std::string &path) const;
	std::vector<DiagnosticItem> forLine(const std::string &path, int line) const;
	// Worst severity per row (0 = none), indexed 0..lineCount-1. One snapshot
	// for render loops — per-line queries copy the whole set each call.
	std::vector<int> maxSeverityByLine(const std::string &path, int lineCount) const;
	bool contains(const std::string &path, int line, int utf16Column) const;

  private:
	// Canonical key for `path`, memoized — the render loop queries per frame
	// and weakly_canonical is a syscall walk.
	std::string keyFor(const std::string &path) const;

	mutable std::mutex mutex_;
	std::unordered_map<std::string, std::vector<DiagnosticItem>> byPath_;
	std::unordered_map<std::string, int> versions_;
	mutable std::unordered_map<std::string, std::string> keyCache_;
};
