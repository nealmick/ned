/*
	No-op EditorGit when NED_ENABLE_GIT=OFF.
	Preserves call sites (gutter, title bar, file tree) without libgit2.
*/

#include "git_service.h"

void EditorGit::init() {}

void EditorGit::onDocumentOpened() { currentGitChanges.clear(); }

void EditorGit::onDidEdit(int, int) {}

void EditorGit::poll() {}

bool EditorGit::isLineEdited(const std::string &, int) const { return false; }

bool EditorGit::isFileModified(const std::string &) const { return false; }
