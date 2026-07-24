/*
	Minimal libgit2 edge: open a repo, read HEAD file as lines, list dirty paths.
	No diffs — the editor owns buffer-vs-HEAD comparison.
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

	// Paths with worktree/index changes (file-tree dots). Slow — call rarely.
	std::set<std::string> modifiedPaths() const;

  private:
	git_repository *repo = nullptr;
	mutable git_tree *headTree = nullptr;
	mutable bool headTreeValid = false;

	bool ensureHeadTree() const;
};
