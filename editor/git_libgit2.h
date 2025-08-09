#pragma once
#include <git2.h>
#include <map>
#include <set>
#include <string>
#include <vector>

struct GitDiffStats
{
	int additions = 0;
	int deletions = 0;
};

struct GitChangedLine
{
	std::string filePath;
	std::vector<int> lines;
};

class GitLibgit2
{
  public:
	GitLibgit2();
	~GitLibgit2();

	// Initialize for a repository path
	bool init(const std::string &repoPath);
	void cleanup();

	// Lock-free git operations using libgit2
	std::map<std::string, std::vector<int>> getEditedLines();
	GitDiffStats getPlusMinusStats(const std::string &filePath);
	std::set<std::string> getModifiedFiles();

	// NEW: Single fast operation to get all data at once
	struct GitAllData
	{
		std::map<std::string, std::vector<int>> editedLines;
		std::map<std::string, GitDiffStats> fileStats;
		std::set<std::string> modifiedFiles;
	};
	GitAllData getAllGitData();

  private:
	git_repository *repo = nullptr;
	std::string repoPath;

	// Helper methods
	bool isRepositoryValid() const;
	git_diff *createDiffToWorkdir();
	void processDiffForEditedLines(git_diff *diff,
								   std::map<std::string, std::vector<int>> &result);
	void
	processDiffForStats(git_diff *diff, const std::string &filePath, GitDiffStats &stats);
	void processDiffForModifiedFiles(git_diff *diff, std::set<std::string> &result);
};