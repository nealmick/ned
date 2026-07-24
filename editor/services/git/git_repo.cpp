#include "git_repo.h"

#include "../../editor_state.h"

#include <git2.h>
#include <mutex>

namespace {

std::once_flag gLibgitOnce;

void ensureLibgit()
{
	std::call_once(gLibgitOnce, [] { git_libgit2_init(); });
}

} // namespace

GitRepo::~GitRepo() { close(); }

void GitRepo::close()
{
	if (headTree)
	{
		git_tree_free(headTree);
		headTree = nullptr;
	}
	headTreeValid = false;
	if (repo)
	{
		git_repository_free(repo);
		repo = nullptr;
	}
}

bool GitRepo::open(const std::string &repoRoot)
{
	close();
	ensureLibgit();
	if (git_repository_open(&repo, repoRoot.c_str()) != 0)
	{
		repo = nullptr;
		return false;
	}
	return true;
}

bool GitRepo::ensureHeadTree() const
{
	if (!repo)
		return false;
	if (headTreeValid)
		return headTree != nullptr;

	headTreeValid = true;
	git_reference *head = nullptr;
	if (git_repository_head(&head, repo) != 0)
		return false;

	const git_oid *oid = git_reference_target(head);
	if (!oid)
	{
		git_reference_free(head);
		return false;
	}

	git_object *obj = nullptr;
	if (git_object_lookup(&obj, repo, oid, GIT_OBJECT_COMMIT) != 0)
	{
		git_reference_free(head);
		return false;
	}
	git_reference_free(head);

	git_tree *tree = nullptr;
	const int err =
		git_object_peel(reinterpret_cast<git_object **>(&tree), obj, GIT_OBJECT_TREE);
	git_object_free(obj);
	if (err != 0)
		return false;

	headTree = tree;
	return true;
}

bool GitRepo::headLines(const std::string &relativePath,
						std::vector<std::string> &out) const
{
	out.clear();
	if (!repo || relativePath.empty())
		return true;

	if (!ensureHeadTree() || !headTree)
		return true; // empty repo / no HEAD → treat as no baseline

	git_tree_entry *entry = nullptr;
	if (git_tree_entry_bypath(&entry, headTree, relativePath.c_str()) != 0)
		return true; // not in HEAD (new file)

	git_blob *blob = nullptr;
	if (git_blob_lookup(&blob, repo, git_tree_entry_id(entry)) != 0)
	{
		git_tree_entry_free(entry);
		return false;
	}
	git_tree_entry_free(entry);

	const char *data = static_cast<const char *>(git_blob_rawcontent(blob));
	const size_t len = static_cast<size_t>(git_blob_rawsize(blob));
	const std::string raw(data, len);
	git_blob_free(blob);

	auto split = EditorState::splitLines(raw);
	out = std::move(split.first);
	return true;
}

std::set<std::string> GitRepo::modifiedPaths() const
{
	std::set<std::string> result;
	if (!repo)
		return result;

	git_status_options opts = GIT_STATUS_OPTIONS_INIT;
	opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
	opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED |
				 GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS |
				 GIT_STATUS_OPT_EXCLUDE_SUBMODULES;

	git_status_list *list = nullptr;
	if (git_status_list_new(&list, repo, &opts) != 0)
		return result;

	const size_t count = git_status_list_entrycount(list);
	for (size_t i = 0; i < count; ++i)
	{
		const git_status_entry *entry = git_status_byindex(list, i);
		if (!entry || entry->status == GIT_STATUS_CURRENT ||
			entry->status == GIT_STATUS_IGNORED)
			continue;

		const char *path = nullptr;
		if (entry->index_to_workdir)
		{
			path = entry->index_to_workdir->new_file.path;
			if (!path || !path[0])
				path = entry->index_to_workdir->old_file.path;
		}
		if ((!path || !path[0]) && entry->head_to_index)
		{
			path = entry->head_to_index->new_file.path;
			if (!path || !path[0])
				path = entry->head_to_index->old_file.path;
		}
		if (path && path[0])
			result.insert(path);
	}
	git_status_list_free(list);
	return result;
}
