#include "save_service.h"
#include "../editor_events.h"
#include "../editor_state.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

namespace {
// Must match the notice prepended by FileExplorer::readFileContent when truncating.
constexpr const char *kTruncatedMarker = "[File truncated - No Edits - showing first";
constexpr size_t kWriteChunk = 64 * 1024;
} // namespace

void EditorSave::setAutosaveIdleMs(int ms) { autosaveIdleMs = std::max(0, ms); }

void EditorSave::save()
{
	// Explicit or timed flush: drop the schedule either way.
	autosavePending = false;

	if (!state || !events || state->path.empty() || !state->dirty)
		return;

	// Truncated-open marker sits at the file start; check a short prefix only.
	{
		const size_t n = std::min(state->byteSize(), size_t{96});
		std::string prefix(n, '\0');
		if (n > 0)
			state->copyBytes(0, n, prefix.data());
		if (prefix.find(kTruncatedMarker) != std::string::npos)
		{
			std::cerr << "Cannot save truncated file content" << std::endl;
			return;
		}
	}

	std::ofstream file(state->path, std::ios::binary);
	if (!file)
	{
		std::cerr << "Unable to save file: " << state->path << std::endl;
		return;
	}

	// Preserve a UTF-8 BOM that was stripped on load (not part of the rope).
	if (state->utf8Bom)
		file << "\xEF\xBB\xBF";

	// Stream rope chunks to disk (no full join).
	const size_t total = state->byteSize();
	std::string buf;
	buf.resize(std::min(kWriteChunk, total > 0 ? total : size_t{1}));
	for (size_t off = 0; off < total;)
	{
		const size_t n = std::min(kWriteChunk, total - off);
		state->copyBytes(off, n, buf.data());
		file.write(buf.data(), static_cast<std::streamsize>(n));
		off += n;
	}
	file.close();
	state->markSaved();
	events->emitDidSave({state->path, state->version});
}

void EditorSave::onDidEdit()
{
	if (!state || state->path.empty() || !state->dirty)
		return;

	autosavePending = true;
	lastEditTime = std::chrono::steady_clock::now();
}

void EditorSave::poll()
{
	if (!autosavePending)
		return;

	if (!state || state->path.empty() || !state->dirty)
	{
		autosavePending = false;
		return;
	}

	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
							 std::chrono::steady_clock::now() - lastEditTime)
							 .count();
	if (elapsed < autosaveIdleMs)
		return;

	save();
}
