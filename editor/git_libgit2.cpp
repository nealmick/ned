#include "git_libgit2.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

GitLibgit2::GitLibgit2()
{
	// Initialize libgit2 globally
	git_libgit2_init();
}

GitLibgit2::~GitLibgit2()
{
	cleanup();
	// Shutdown libgit2 globally
	git_libgit2_shutdown();
}

bool GitLibgit2::init(const std::string &path)
{
	cleanup(); // Clean up any existing repository

	repoPath = path;

	// Open the repository
	int error = git_repository_open(&repo, path.c_str());
	if (error != 0)
	{
		std::cerr << "Failed to open git repository: " << git_error_last()->message
				  << std::endl;
		return false;
	}

	return true;
}

void GitLibgit2::cleanup()
{
	if (repo)
	{
		git_repository_free(repo);
		repo = nullptr;
	}
}

bool GitLibgit2::isRepositoryValid() const { return repo != nullptr; }

git_diff *GitLibgit2::createDiffToWorkdir()
{
	if (!isRepositoryValid())
	{
		return nullptr;
	}

	git_diff *diff = nullptr;
	git_diff_options opts = GIT_DIFF_OPTIONS_INIT;

	// Create diff between HEAD and working directory
	int error = git_diff_index_to_workdir(&diff, repo, nullptr, &opts);
	if (error != 0)
	{
		std::cerr << "Failed to create diff: " << git_error_last()->message << std::endl;
		return nullptr;
	}

	return diff;
}

std::map<std::string, std::vector<int>> GitLibgit2::getEditedLines()
{
	std::map<std::string, std::vector<int>> result;

	if (!isRepositoryValid())
	{
		return result;
	}

	git_diff *diff = createDiffToWorkdir();
	if (!diff)
	{
		return result;
	}

	processDiffForEditedLines(diff, result);
	git_diff_free(diff);

	return result;
}

void GitLibgit2::processDiffForEditedLines(git_diff *diff,
										   std::map<std::string, std::vector<int>> &result)
{
	// Use libgit2's diff print to get line-by-line data
	git_diff_print(
		diff,
		GIT_DIFF_FORMAT_PATCH,
		[](const git_diff_delta *delta,
		   const git_diff_hunk *hunk,
		   const git_diff_line *line,
		   void *payload) -> int {
			if (line->origin == GIT_DIFF_LINE_ADDITION)
			{
				auto *result_map =
					static_cast<std::map<std::string, std::vector<int>> *>(payload);
				std::string file_path = delta->new_file.path;
				(*result_map)[file_path].push_back(line->new_lineno);
			}
			return 0;
		},
		&result);
}

GitDiffStats GitLibgit2::getPlusMinusStats(const std::string &filePath)
{
	GitDiffStats stats;

	if (!isRepositoryValid())
	{
		return stats;
	}

	git_diff *diff = createDiffToWorkdir();
	if (!diff)
	{
		return stats;
	}

	processDiffForStats(diff, filePath, stats);
	git_diff_free(diff);

	return stats;
}

void GitLibgit2::processDiffForStats(git_diff *diff,
									 const std::string &filePath,
									 GitDiffStats &stats)
{
	// Count additions/deletions by iterating through diff lines for the specific file
	auto context = std::make_pair(&filePath, &stats);
	git_diff_print(
		diff,
		GIT_DIFF_FORMAT_PATCH,
		[](const git_diff_delta *delta,
		   const git_diff_hunk *hunk,
		   const git_diff_line *line,
		   void *payload) -> int {
			auto *context_ptr =
				static_cast<std::pair<const std::string *, GitDiffStats *> *>(payload);
			const std::string &target_file = *context_ptr->first;
			GitDiffStats &file_stats = *context_ptr->second;

			// Only count lines for the target file
			if (std::string(delta->new_file.path) == target_file)
			{
				if (line->origin == GIT_DIFF_LINE_ADDITION)
				{
					file_stats.additions++;
				} else if (line->origin == GIT_DIFF_LINE_DELETION)
				{
					file_stats.deletions++;
				}
			}
			return 0;
		},
		&context);
}

std::set<std::string> GitLibgit2::getModifiedFiles()
{
	std::set<std::string> result;

	if (!isRepositoryValid())
	{
		return result;
	}

	git_diff *diff = createDiffToWorkdir();
	if (!diff)
	{
		return result;
	}

	processDiffForModifiedFiles(diff, result);
	git_diff_free(diff);

	return result;
}

void GitLibgit2::processDiffForModifiedFiles(git_diff *diff, std::set<std::string> &result)
{
	size_t num_deltas = git_diff_num_deltas(diff);

	for (size_t i = 0; i < num_deltas; ++i)
	{
		const git_diff_delta *delta = git_diff_get_delta(diff, i);

		// Add modified files to the result set
		if (delta->status == GIT_DELTA_MODIFIED || delta->status == GIT_DELTA_ADDED ||
			delta->status == GIT_DELTA_DELETED || delta->status == GIT_DELTA_RENAMED ||
			delta->status == GIT_DELTA_COPIED)
		{
			result.insert(delta->new_file.path);
		}
	}
}

GitLibgit2::GitAllData GitLibgit2::getAllGitData()
{
	GitAllData result;

	if (!isRepositoryValid())
	{
		return result;
	}

	git_diff *diff = createDiffToWorkdir();
	if (!diff)
	{
		return result;
	}

	// Get all data from the single diff
	processDiffForEditedLines(diff, result.editedLines);
	processDiffForModifiedFiles(diff, result.modifiedFiles);

	// Get stats for each file
	git_diff_print(
		diff,
		GIT_DIFF_FORMAT_PATCH,
		[](const git_diff_delta *delta,
		   const git_diff_hunk *hunk,
		   const git_diff_line *line,
		   void *payload) -> int {
			auto *result_ptr = static_cast<GitAllData *>(payload);
			std::string file_path = delta->new_file.path;

			if (line->origin == GIT_DIFF_LINE_ADDITION)
			{
				result_ptr->fileStats[file_path].additions++;
			} else if (line->origin == GIT_DIFF_LINE_DELETION)
			{
				result_ptr->fileStats[file_path].deletions++;
			}
			return 0;
		},
		&result);

	git_diff_free(diff);
	return result;
}