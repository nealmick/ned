#include "git_libgit2.h"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

namespace fs = std::filesystem;

// Timing macros removed for clean output

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
	invalidateTreeCache();
	if (repo)
	{
		git_repository_free(repo);
		repo = nullptr;
	}
}

bool GitLibgit2::isRepositoryValid() const { return repo != nullptr; }

void GitLibgit2::invalidateTreeCache()
{
	if (cachedHeadTree)
	{
		git_tree_free(cachedHeadTree);
		cachedHeadTree = nullptr;
	}
	hasCachedTree = false;
}

git_tree *GitLibgit2::getCachedHeadTree()
{
	if (!isRepositoryValid())
	{
		return nullptr;
	}

	// Get current HEAD OID
	git_oid currentHeadOid;
	git_reference *head_ref = nullptr;
	int error = git_repository_head(&head_ref, repo);
	if (error != 0)
	{
		return nullptr; // No HEAD
	}

	const git_oid *target_oid = git_reference_target(head_ref);
	if (!target_oid)
	{
		git_reference_free(head_ref);
		return nullptr;
	}

	currentHeadOid = *target_oid;
	git_reference_free(head_ref);

	// Check if we have a valid cached tree for this HEAD
	if (hasCachedTree && git_oid_equal(&currentHeadOid, &cachedHeadOid))
	{
		return cachedHeadTree; // Return cached tree
	}

	// Cache is invalid, need to load new tree
	invalidateTreeCache();

	git_object *head_obj = nullptr;
	error = git_object_lookup(&head_obj, repo, &currentHeadOid, GIT_OBJECT_COMMIT);
	if (error == 0)
	{
		error =
			git_object_peel((git_object **)&cachedHeadTree, head_obj, GIT_OBJECT_TREE);
		if (error == 0)
		{
			cachedHeadOid = currentHeadOid;
			hasCachedTree = true;
		}
		git_object_free(head_obj);
	}

	return (error == 0) ? cachedHeadTree : nullptr;
}

git_diff *GitLibgit2::createSingleFileDiff(const std::string &filePath)
{

	if (!isRepositoryValid())
	{
		return nullptr;
	}

	git_diff *diff = nullptr;
	git_diff_options opts = GIT_DIFF_OPTIONS_INIT;

	// Optimize for single file - no binary checks, no context
	opts.flags = GIT_DIFF_SKIP_BINARY_CHECK;
	opts.context_lines = 0;
	opts.interhunk_lines = 0;

	// Only diff the specific file
	const char *pathspec[] = {filePath.c_str()};
	git_strarray paths;
	paths.strings = (char **)pathspec;
	paths.count = 1;
	opts.pathspec = paths;

	// Get cached HEAD tree
	git_tree *head_tree = getCachedHeadTree();

	// Create diff for just this file
	int error = git_diff_tree_to_workdir(&diff, repo, head_tree, &opts);

	if (error != 0)
	{
		std::cerr << "Failed to create single file diff: " << git_error_last()->message
				  << std::endl;
		return nullptr;
	}

	return diff;
}

std::vector<int> GitLibgit2::getCurrentFileEditedLines(const std::string &filePath)
{
	std::vector<int> result;

	git_diff *diff = createSingleFileDiff(filePath);
	if (!diff)
	{
		return result;
	}

	// Process diff to get edited lines
	git_diff_print(
		diff,
		GIT_DIFF_FORMAT_PATCH,
		[](const git_diff_delta *delta,
		   const git_diff_hunk *hunk,
		   const git_diff_line *line,
		   void *payload) -> int {
			if (line->origin == GIT_DIFF_LINE_ADDITION)
			{
				auto *result_vec = static_cast<std::vector<int> *>(payload);
				result_vec->push_back(line->new_lineno);
			}
			return 0;
		},
		&result);

	git_diff_free(diff);
	return result;
}

GitDiffStats GitLibgit2::getCurrentFileStats(const std::string &filePath)
{
	GitDiffStats stats;

	git_diff *diff = createSingleFileDiff(filePath);
	if (!diff)
	{
		return stats;
	}

	// Process diff to get stats
	git_diff_print(
		diff,
		GIT_DIFF_FORMAT_PATCH,
		[](const git_diff_delta *delta,
		   const git_diff_hunk *hunk,
		   const git_diff_line *line,
		   void *payload) -> int {
			auto *file_stats = static_cast<GitDiffStats *>(payload);
			if (line->origin == GIT_DIFF_LINE_ADDITION)
			{
				file_stats->additions++;
			} else if (line->origin == GIT_DIFF_LINE_DELETION)
			{
				file_stats->deletions++;
			}
			return 0;
		},
		&stats);

	git_diff_free(diff);
	return stats;
}

GitLibgit2::CurrentFileData GitLibgit2::getCurrentFileData(const std::string &filePath)
{
	CurrentFileData result;

	git_diff *diff = createSingleFileDiff(filePath);
	if (!diff)
	{
		return result;
	}

	// Process diff to get both lines and stats in single pass
	git_diff_print(
		diff,
		GIT_DIFF_FORMAT_PATCH,
		[](const git_diff_delta *delta,
		   const git_diff_hunk *hunk,
		   const git_diff_line *line,
		   void *payload) -> int {
			auto *data = static_cast<CurrentFileData *>(payload);
			if (line->origin == GIT_DIFF_LINE_ADDITION)
			{
				data->editedLines.push_back(line->new_lineno);
				data->stats.additions++;
			} else if (line->origin == GIT_DIFF_LINE_DELETION)
			{
				data->stats.deletions++;
			}
			return 0;
		},
		&result);

	git_diff_free(diff);
	return result;
}

git_diff *GitLibgit2::createDiffToWorkdir()
{

	if (!isRepositoryValid())
	{
		return nullptr;
	}

	git_diff *diff = nullptr;
	git_diff_options opts = GIT_DIFF_OPTIONS_INIT;

	// Optimize for speed - remove expensive operations and add exclusions
	opts.flags = GIT_DIFF_SKIP_BINARY_CHECK | GIT_DIFF_INCLUDE_UNMODIFIED;
	opts.context_lines = 0;		// No context lines needed for our use case
	opts.interhunk_lines = 0;	// No interhunk lines needed
	opts.max_size = 512 * 1024; // Skip very large files (>512KB) for speed

	// Exclude common directories that slow down scanning
	const char *pathspec[] = {":!node_modules/*",
							  ":!.git/*",
							  ":!build/*",
							  ":!dist/*",
							  ":!target/*",
							  ":!vendor/*",
							  ":!venv/*",
							  ":!.venv/*",
							  ":!__pycache__/*",
							  ":!*.pyc",
							  ":!.DS_Store"};
	git_strarray paths;
	paths.strings = (char **)pathspec;
	paths.count = sizeof(pathspec) / sizeof(pathspec[0]);
	opts.pathspec = paths;

	// Get cached HEAD tree for comparison
	git_tree *head_tree = getCachedHeadTree();

	// Compare HEAD tree to working directory with optimized options
	int error = git_diff_tree_to_workdir(&diff, repo, head_tree, &opts);

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

	// Get modified files first (fastest operation)
	processDiffForModifiedFiles(diff, result.modifiedFiles);

	// Single optimized pass to get both edited lines AND stats
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
				// Collect edited lines AND increment stats in single pass
				result_ptr->editedLines[file_path].push_back(line->new_lineno);
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