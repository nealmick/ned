#include "git_service.h"
#include "line_diff.h"

#include "../../../util/settings.h"
#include "../../editor_state.h"

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

bool EditorGit::linesEnabled() const
{
	return !settings || settings->settings.value("git_changed_lines", true);
}

std::string EditorGit::relativePath(const std::string &abs) const
{
	if (!projectRoot || projectRoot->empty() || abs.empty())
		return {};

	std::error_code ec;
	const fs::path root = fs::weakly_canonical(*projectRoot, ec);
	const fs::path file = fs::weakly_canonical(abs, ec);
	if (ec)
		return {};

	const auto rel = fs::relative(file, root, ec);
	if (ec || rel.empty() || *rel.begin() == "..")
		return {};

	std::string s = rel.generic_string();
	return s;
}

void EditorGit::clearGutter()
{
	dirtyLines.clear();
	currentGitChanges.clear();
	baseline.clear();
	baselinePath.clear();
	currentLines.clear();
}

void EditorGit::init()
{
	clearGutter();
	modifiedFiles.clear();
	repo.close();

	if (!projectRoot || projectRoot->empty())
		return;

	if (!fs::exists(fs::path(*projectRoot) / ".git"))
		return;

	if (!repo.open(*projectRoot))
		return;

	refreshStatus();
	lastStatus = std::chrono::steady_clock::now();

	// If a file is already open, seed baseline + gutter.
	if (state && !state->path.empty())
		onDocumentOpened();
}

void EditorGit::loadBaseline()
{
	baseline.clear();
	baselinePath.clear();
	if (!state || state->path.empty() || !repo.isOpen())
		return;

	const std::string rel = relativePath(state->path);
	if (rel.empty())
		return;

	repo.headLines(rel, baseline);
	baselinePath = state->path;
}

void EditorGit::rebuildLineCache()
{
	currentLines.clear();
	if (!state)
		return;
	state->linesInto(currentLines);
}

bool EditorGit::syncLineCacheFromEdit(int firstRow, int lastRow)
{
	if (!state)
		return false;

	const int newN = state->lineCount();
	if (newN <= 0)
	{
		currentLines.clear();
		return true;
	}

	// Cold cache or huge jump (e.g. replace-all) — full materialize once.
	const int oldN = static_cast<int>(currentLines.size());
	if (oldN == 0 || std::abs(newN - oldN) > 500)
	{
		rebuildLineCache();
		return static_cast<int>(currentLines.size()) == newN;
	}

	int lo = std::clamp(firstRow, 0, newN - 1);
	int hi = std::clamp(lastRow, 0, newN - 1);
	if (lo > hi)
		std::swap(lo, hi);

	const int delta = newN - oldN;
	const int newDirty = hi - lo + 1;
	const int oldDirty = newDirty - delta;

	// Span must describe a coherent replace of oldDirty lines with newDirty lines.
	if (oldDirty < 0 || lo + oldDirty > oldN)
	{
		rebuildLineCache();
		return static_cast<int>(currentLines.size()) == newN;
	}

	// Splice: drop the pre-edit dirty lines, insert post-edit slots, then fill.
	if (oldDirty > 0)
	{
		currentLines.erase(currentLines.begin() + lo,
						   currentLines.begin() + lo + oldDirty);
	}
	if (newDirty > 0)
	{
		currentLines.insert(
			currentLines.begin() + lo, static_cast<size_t>(newDirty), std::string{});
	}

	if (static_cast<int>(currentLines.size()) != newN)
	{
		rebuildLineCache();
		return static_cast<int>(currentLines.size()) == newN;
	}

	for (int r = lo; r <= hi; ++r)
		state->lineInto(r, currentLines[static_cast<size_t>(r)]);

	return true;
}

void EditorGit::recomputeGutterFromCache()
{
	dirtyLines.clear();
	currentGitChanges.clear();

	if (!linesEnabled() || !state || state->path.empty())
		return;

	if (baselinePath != state->path)
		loadBaseline();

	// Ensure cache matches document line count.
	if (static_cast<int>(currentLines.size()) != state->lineCount())
		rebuildLineCache();

	const LineDiff d = diffLines(baseline, currentLines);
	dirtyLines = std::move(d.addedLines);

	if (d.additions > 0 || d.deletions > 0)
	{
		currentGitChanges =
			"+" + std::to_string(d.additions) + "-" + std::to_string(d.deletions);
	}
}

void EditorGit::onDocumentOpened()
{
	loadBaseline();
	rebuildLineCache();
	recomputeGutterFromCache();
}

void EditorGit::onDidEdit(int firstRow, int lastRow)
{
	// Immediate gutter update — no debounce. Only the dirty line span is
	// re-read from the buffer; then LCS runs on the cached line arrays.
	if (!linesEnabled() || !state)
		return;

	if (baselinePath != state->path)
		loadBaseline();

	if (!syncLineCacheFromEdit(firstRow, lastRow))
		rebuildLineCache();

	recomputeGutterFromCache();
}

void EditorGit::refreshStatus()
{
	if (!repo.isOpen())
	{
		modifiedFiles.clear();
		return;
	}
	modifiedFiles = repo.modifiedPaths();
}

void EditorGit::poll()
{
	if (!repo.isOpen())
		return;

	const auto now = std::chrono::steady_clock::now();
	const auto ms =
		std::chrono::duration_cast<std::chrono::milliseconds>(now - lastStatus).count();
	if (ms < kStatusIntervalMs)
		return;

	refreshStatus();
	lastStatus = now;
}

bool EditorGit::isLineEdited(const std::string &filePath, int lineNumber) const
{
	if (!state || filePath != state->path)
		return false;
	return dirtyLines.count(lineNumber) > 0;
}

bool EditorGit::isFileModified(const std::string &filePath) const
{
	if (!repo.isOpen())
		return false;
	const std::string rel = relativePath(filePath);
	if (rel.empty())
		return false;
	return modifiedFiles.count(rel) > 0;
}
