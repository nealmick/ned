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

bool GitRepo::indexLines(const std::string &relativePath,
						 std::vector<std::string> &out) const
{
	out.clear();
	if (!repo || relativePath.empty())
		return true;

	git_index *index = nullptr;
	if (git_repository_index(&index, repo) != 0)
		return false;

	const git_index_entry *entry = git_index_get_bypath(index, relativePath.c_str(), 0);
	if (!entry)
	{
		git_index_free(index);
		return true; // not staged → empty baseline (e.g. untracked file)
	}

	git_blob *blob = nullptr;
	if (git_blob_lookup(&blob, repo, &entry->id) != 0)
	{
		git_index_free(index);
		return false;
	}

	const char *data = static_cast<const char *>(git_blob_rawcontent(blob));
	const size_t len = static_cast<size_t>(git_blob_rawsize(blob));
	const std::string raw(data, len);
	git_blob_free(blob);
	git_index_free(index);

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

void GitRepo::statusGroups(std::vector<std::string> &staged,
						   std::vector<std::string> &unstaged) const
{
	staged.clear();
	unstaged.clear();
	if (!repo)
		return;

	git_status_options opts = GIT_STATUS_OPTIONS_INIT;
	opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
	opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED |
				 GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS |
				 GIT_STATUS_OPT_EXCLUDE_SUBMODULES;

	git_status_list *list = nullptr;
	if (git_status_list_new(&list, repo, &opts) != 0)
		return;

	const size_t count = git_status_list_entrycount(list);
	for (size_t i = 0; i < count; ++i)
	{
		const git_status_entry *entry = git_status_byindex(list, i);
		if (!entry || entry->status == GIT_STATUS_CURRENT ||
			entry->status == GIT_STATUS_IGNORED)
			continue;

		// Prefer the worktree path spelling (it's what the user sees on
		// disk); fall back to the index spelling for staged-only entries.
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
		if (!path || !path[0])
			continue;

		// INDEX_* bits = staged (HEAD vs index); WT_* bits + untracked =
		// unstaged (index vs worktree). A path can be in both.
		const unsigned s = entry->status;
		if (s &
			(GIT_STATUS_INDEX_NEW | GIT_STATUS_INDEX_MODIFIED | GIT_STATUS_INDEX_DELETED |
			 GIT_STATUS_INDEX_RENAMED | GIT_STATUS_INDEX_TYPECHANGE))
			staged.push_back(path);
		if (s & (GIT_STATUS_WT_NEW | GIT_STATUS_WT_MODIFIED | GIT_STATUS_WT_DELETED |
				 GIT_STATUS_WT_RENAMED | GIT_STATUS_WT_TYPECHANGE))
			unstaged.push_back(path);
	}
	git_status_list_free(list);
}

std::vector<GitRepo::Commit> GitRepo::history(size_t limit) const
{
	std::vector<Commit> commits;
	if (!repo)
		return commits;

	git_revwalk *walk = nullptr;
	if (git_revwalk_new(&walk, repo) != 0)
		return commits;
	git_revwalk_sorting(walk, GIT_SORT_TOPOLOGICAL | GIT_SORT_TIME);
	if (git_revwalk_push_head(walk) != 0)
	{
		git_revwalk_free(walk);
		return commits; // empty repo (unborn HEAD)
	}

	git_oid oid;
	while (commits.size() < limit && git_revwalk_next(&oid, walk) == 0)
	{
		git_commit *commit = nullptr;
		if (git_commit_lookup(&commit, repo, &oid) != 0)
			continue;

		Commit c;
		char hex[GIT_OID_HEXSZ + 1] = {0};
		git_oid_fmt(hex, &oid);
		c.id = hex;
		c.shortId = c.id.substr(0, 7);
		const char *summary = git_commit_summary(commit);
		c.summary = summary ? summary : "";
		const unsigned parents = git_commit_parentcount(commit);
		for (unsigned p = 0; p < parents && p < 4; ++p)
		{
			char phex[GIT_OID_HEXSZ + 1] = {0};
			git_oid_fmt(phex, git_commit_parent_id(commit, p));
			c.parentIds.push_back(phex);
		}
		git_commit_free(commit);
		commits.push_back(std::move(c));
	}
	git_revwalk_free(walk);
	return commits;
}

bool GitRepo::remoteUrl(std::string &out) const
{
	out.clear();
	if (!repo)
		return false;
	git_remote *remote = nullptr;
	if (git_remote_lookup(&remote, repo, "origin") != 0)
	{
		git_strarray names = {nullptr, 0};
		if (git_remote_list(&names, repo) != 0 || names.count == 0)
		{
			git_strarray_dispose(&names);
			return false;
		}
		if (git_remote_lookup(&remote, repo, names.strings[0]) != 0)
		{
			git_strarray_dispose(&names);
			return false;
		}
		git_strarray_dispose(&names);
	}
	const char *url = git_remote_url(remote);
	if (url)
		out = url;
	git_remote_free(remote);
	return !out.empty();
}

bool GitRepo::commitDetails(const std::string &idHex,
							std::string &message,
							std::vector<FileStat> &files) const
{
	message.clear();
	files.clear();
	if (!repo || idHex.size() < 7)
		return false;

	git_oid oid;
	if (git_oid_fromstrp(&oid, idHex.c_str()) != 0)
		return false;

	git_commit *commit = nullptr;
	if (git_commit_lookup(&commit, repo, &oid) != 0)
		return false;

	const char *msg = git_commit_message(commit);
	message = msg ? msg : "";

	// Parent tree (first parent; the initial commit diffs against the
	// empty tree) → this commit's tree.
	git_object *peel = nullptr;
	if (git_object_peel(
			&peel, reinterpret_cast<const git_object *>(commit), GIT_OBJECT_TREE) != 0)
	{
		git_commit_free(commit);
		return false;
	}
	git_tree *tree = reinterpret_cast<git_tree *>(peel);

	git_tree *parentTree = nullptr;
	if (git_commit_parentcount(commit) > 0)
	{
		git_commit *parent = nullptr;
		if (git_commit_parent(&parent, commit, 0) == 0)
		{
			git_object *parentPeel = nullptr;
			if (git_object_peel(&parentPeel,
								reinterpret_cast<const git_object *>(parent),
								GIT_OBJECT_TREE) == 0)
				parentTree = reinterpret_cast<git_tree *>(parentPeel);
			git_commit_free(parent);
		}
	}

	git_diff *diff = nullptr;
	git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
	if (git_diff_tree_to_tree(&diff, repo, parentTree, tree, &opts) == 0)
	{
		const size_t count = git_diff_num_deltas(diff);
		for (size_t i = 0; i < count; ++i)
		{
			git_patch *patch = nullptr;
			if (git_patch_from_diff(&patch, diff, i) != 0 || !patch)
				continue;
			FileStat fs;
			const git_diff_delta *delta = git_patch_get_delta(patch);
			if (delta)
				fs.path = delta->new_file.path
							  ? delta->new_file.path
							  : (delta->old_file.path ? delta->old_file.path : "");
			size_t context = 0, additions = 0, deletions = 0;
			git_patch_line_stats(&context, &additions, &deletions, patch);
			fs.added = (int)additions;
			fs.deleted = (int)deletions;
			git_patch_free(patch);
			if (!fs.path.empty())
				files.push_back(std::move(fs));
		}
		git_diff_free(diff);
	}

	if (parentTree)
		git_tree_free(parentTree);
	git_tree_free(tree);
	git_commit_free(commit);
	return true;
}
