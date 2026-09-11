/*
	libgit2 edge for the editor + the git panel: repo open, HEAD/index line
	reads (editor gutter baselines + diff views), status groups, history,
	commit details and the remote URL. Line-level diffING stays in
	line_diff/EditorGit — this class only moves raw git data, always off
	the UI thread for the slow calls.
*/

#pragma once

#include <set>
#include <string>
#include <vector>

struct git_repository;
struct git_tree;

class GitRepo
{
  public:
	GitRepo() = default;
	~GitRepo();

	GitRepo(const GitRepo &) = delete;
	GitRepo &operator=(const GitRepo &) = delete;

	bool open(const std::string &repoRoot);
	void close();
	bool isOpen() const { return repo != nullptr; }

	// Lines of path at HEAD (same split rules as EditorState). Empty if untracked.
	// Returns false only on hard failure; missing path → true + empty lines.
	bool headLines(const std::string &relativePath, std::vector<std::string> &out) const;

	// Lines of path as STAGED in the index (same contract: missing → true +
	// empty, false only on hard failure). The diff view's baseline for
	// unstaged changes; combined with headLines it also builds the staged
	// (HEAD vs index) diff.
	bool indexLines(const std::string &relativePath, std::vector<std::string> &out) const;

	// Paths with worktree/index changes (file-tree dots). Slow — call rarely.
	std::set<std::string> modifiedPaths() const;

	// Git-browser queries (hosted panel). Same "slow, call off the UI
	// thread" contract as modifiedPaths — the panel owns the worker.

	// One status pass split VSCode-style: index changes → staged, worktree
	// changes (incl. untracked) → unstaged. Paths are repo-relative.
	void statusGroups(std::vector<std::string> &staged,
					  std::vector<std::string> &unstaged) const;

	// One commit of the branch history, newest first.
	struct Commit
	{
		std::string id;		 // full oid hex (graph lane bookkeeping)
		std::string shortId; // first 7 chars
		std::string summary; // first line of the message
		std::vector<std::string> parentIds;
	};
	// Up to `limit` commits reachable from HEAD. Empty on failure/no repo.
	std::vector<Commit> history(size_t limit) const;

	// Per-file change counts of one commit (graph hover popup).
	struct FileStat
	{
		std::string path;
		int added = 0;
		int deleted = 0;
	};
	// First remote's URL (origin preferred). False when no remote.
	bool remoteUrl(std::string &out) const;

	// Full message + changed files with +/- counts. False on lookup
	// failure. Slow (patch walk) — fetch on demand, never per-frame.
	bool commitDetails(const std::string &idHex,
					   std::string &message,
					   std::vector<FileStat> &files) const;

  private:
	git_repository *repo = nullptr;
	mutable git_tree *headTree = nullptr;
	mutable bool headTreeValid = false;

	bool ensureHeadTree() const;
};
