/*
	File: services/highlight/highlight_service.h
	Description: Syntax colors as per-line non-overlapping byte ranges.

	On edit: morphSpans (geometry, UI thread, instant) then tree-sitter recolor.
	Small files recolor synchronously; large files recolor on a worker. The full
	lineStarts rope walk runs inside parse() (worker for large files), not on the
	keystroke path beyond CoW snapshot + morph.
*/

#pragma once
#include "imgui.h"
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
	// Above this, recolor on a worker (avoid hitching the UI thread).
	static constexpr size_t kAsyncRecolorBytes = 512 * 1024;
	static constexpr size_t kSkipTreeSitterBytes = 100 * 1024 * 1024;

	EditorHighlight(EditorState &document, EditorOperations &ops, Settings &appSettings);

	// Morph pending ops, then recolor (sync unless file is large).
	void highlightContent();
	void cancelHighlighting();
	void forceColorUpdate();
	void poll(); // adopt finished async recolor (large files only)

	ImVec4 defaultTextColor() const;
	const LineColorSpans &spansForLine(int row) const;

	void resetForDocument(size_t lineCount);
	void clear();

  private:
	EditorState *state;
	EditorOperations *operations;
	Settings *settings;
	TreeSitter treeSitter;

	ColorRangeMap lineColors;
	std::vector<int> lineLens; // pre-edit line sizes for morphDelete walks
	static const LineColorSpans kEmptySpans;

	std::shared_ptr<std::atomic_bool> cancelFlag;
	std::vector<std::future<void>> tasks;
	uint64_t jobGen = 0;

	std::mutex pendingMutex;
	std::shared_ptr<ParseResult> pendingResult;
	uint64_t pendingGen = 0;

	std::vector<PendingTreeEdit> heldTreeEdits;

	void morphSpans(const std::vector<PendingTreeEdit> &edits);
	void morphInsert(int row, int col, const std::string &text);
	void morphDelete(int row, int col, int length);
	void syncLensFromContent();
	static void insertBytes(LineColorSpans &spans, int col, int n);
	static void deleteBytes(LineColorSpans &spans, int col, int n);

	void reapTasks();
	void recolor();
	TreeSitter::ParseSnapshot makeSnapshot(std::vector<PendingTreeEdit> pending) const;
	void applyParseResult(ParseResult &result);
	void publishPending();
	size_t contentByteSize() const;
};
