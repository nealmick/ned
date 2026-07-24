/*
	Git gutter + file-tree dirty marks.

	Model:
	  - Load HEAD lines once when a document opens (baseline).
	  - Keep a live line cache; on edit, splice only the dirty span (no full
		linesInto). Diff cache vs baseline immediately for gutter + "+N-M".
	  - Rarely: git status for explorer dots.

	libgit2 is only used for baseline blob + status — never on the typing path.
*/
#pragma once

#include "git_repo.h"

#include <chrono>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

class EditorState;
class Settings;

class EditorGit
{
  public:
	EditorGit(EditorState &document, std::string &projectRootRef, Settings &appSettings)
		: state(&document), projectRoot(&projectRootRef), settings(&appSettings)
	{
	}

	// Project root changed / opened.
	void init();

	// Document path/content installed (after openDocument / setContent).
	void onDocumentOpened();

	// Content mutated — update line cache for the dirty span and re-diff now.
	void onDidEdit(int firstRow, int lastRow);

	// Frame tick: optional status refresh for the file tree.
	void poll();

	bool isLineEdited(const std::string &filePath, int lineNumber) const;
	bool isFileModified(const std::string &filePath) const;

	// Title bar "+N-M" (empty if clean / disabled).
	std::string currentGitChanges;

  private:
	EditorState *state;
	std::string *projectRoot;
	Settings *settings;

	GitRepo repo;
	std::vector<std::string> baseline; // HEAD lines for the open path
	std::string baselinePath;		   // absolute path baseline belongs to
	// Live document lines — updated incrementally on edit (not rebuilt each time).
	std::vector<std::string> currentLines;
	std::unordered_set<int> dirtyLines; // 1-based
	std::set<std::string> modifiedFiles;

	std::chrono::steady_clock::time_point lastStatus{};
	static constexpr int kStatusIntervalMs = 1000;

	bool linesEnabled() const;
	std::string relativePath(const std::string &abs) const;
	void clearGutter();
	void loadBaseline();
	// Full rebuild of currentLines from the document (open / desync recovery).
	void rebuildLineCache();
	// Splice currentLines for a DidEdit span, then fill those rows from state.
	bool syncLineCacheFromEdit(int firstRow, int lastRow);
	void recomputeGutterFromCache();
	void refreshStatus();
};
