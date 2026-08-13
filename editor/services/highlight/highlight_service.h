/*
	File: services/highlight/highlight_service.h
	Description: Per-line syntax spans (ThemeSlot, not RGB).

	Edit: morph geometry on this thread, then recolor.
	Open: cheap prefix query here; full parse + rest on a worker.
	Theme: remap palette only.
*/

#pragma once
#include "imgui.h"
#include "span_map.h"
#include "tree_sitter.h"
#include <atomic>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class EditorState;
class EditorOperations;
class Settings;

class EditorHighlight
{
  public:
	// Incremental edits at or below this stay on the UI thread.
	static constexpr size_t kSyncIncrementalBytes = 16 * 1024;
	static constexpr size_t kSkipTreeSitterBytes = 100 * 1024 * 1024;
	static constexpr int kPrimeQueryLines = 128;
	static constexpr int kQueryChunkLines = 384;

	EditorHighlight(EditorState &document,
					EditorOperations &ops,
					Settings *appSettings = nullptr);

	void highlightContent();
	void cancelHighlighting();
	void forceColorUpdate();
	void poll();

	ImVec4 defaultTextColor() const;
	ImVec4 colorForSlot(ThemeSlot slot) const;
	const LineColorSpans &spansForLine(int row) const;

	// Bumps when span map or theme colors change — minimap/other caches key off this.
	uint64_t visualGeneration() const { return visualGen_; }

	void resetForDocument(size_t lineCount);
	void clear();

  private:
	EditorState *state;
	EditorOperations *operations;
	Settings *settings;
	TreeSitter treeSitter;
	SpanMap spans;
	uint64_t visualGen_ = 1;

	std::shared_ptr<std::atomic_bool> cancelFlag;
	std::vector<std::future<void>> tasks;
	uint64_t jobGen = 0;

	std::mutex pendingMutex;
	std::shared_ptr<ParseResult> pendingResult;
	uint64_t pendingGen = 0;
	std::vector<PendingTreeEdit> heldTreeEdits;

	void morphSpans(const std::vector<PendingTreeEdit> &edits);
	void syncLensFromContent();
	void reapTasks();
	void recolor();
	TreeSitter::ParseSnapshot makeSnapshot(std::vector<PendingTreeEdit> pending) const;
	void applyParseResult(ParseResult &result);
	void postResult(ParseResult result, uint64_t gen);
	void publishPending();
	void runJob(TreeSitter::ParseSnapshot &snap,
				uint64_t gen,
				size_t lineCount,
				const std::shared_ptr<std::atomic_bool> &canceled);
	void launchJob(TreeSitter::ParseSnapshot snap, uint64_t gen, size_t lineCount);
};
