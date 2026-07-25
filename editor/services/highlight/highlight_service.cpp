#include "highlight_service.h"
#include "../../../util/settings.h"
#include "../../editor_operations.h"
#include "../../editor_state.h"
#include "../../util/editor_utils.h"

#include <algorithm>

const LineColorSpans EditorHighlight::kEmptySpans{};

EditorHighlight::EditorHighlight(EditorState &document,
								 EditorOperations &ops,
								 Settings &appSettings)
	: state(&document), operations(&ops), settings(&appSettings)
{
	treeSitter.bind(appSettings);
	// Load theme on construct so new editors never paint transparent text.
	treeSitter.updateThemeColors();
}

void EditorHighlight::cancelHighlighting()
{
	if (cancelFlag)
		cancelFlag->store(true);
	reapTasks();
}

void EditorHighlight::reapTasks()
{
	std::erase_if(tasks, [](std::future<void> &t) {
		if (t.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
			return false;
		t.get();
		return true;
	});
}

void EditorHighlight::forceColorUpdate()
{
	cancelHighlighting();
	treeSitter.updateThemeColors();
	if (state && !state->path.empty())
		highlightContent();
}

ImVec4 EditorHighlight::defaultTextColor() const { return treeSitter.cachedColors.text; }

void EditorHighlight::syncLensFromContent()
{
	lineLens.clear();
	if (!state)
		return;
	const int n = state->lineCount();
	lineLens.reserve(static_cast<size_t>(n));
	for (int i = 0; i < n; ++i)
		lineLens.push_back(state->lineLength(i));
}

void EditorHighlight::clear()
{
	cancelHighlighting();
	++jobGen;
	heldTreeEdits.clear();
	lineColors.clear();
	lineLens.clear();
	std::lock_guard lock(pendingMutex);
	pendingResult.reset();
	pendingGen = 0;
}

void EditorHighlight::resetForDocument(size_t lineCount)
{
	cancelHighlighting();
	++jobGen;
	heldTreeEdits.clear();
	lineColors.assign(lineCount, {});
	syncLensFromContent();
	if (lineLens.size() != lineCount)
		lineLens.assign(lineCount, 0);
	std::lock_guard lock(pendingMutex);
	pendingResult.reset();
	pendingGen = 0;
}

const LineColorSpans &EditorHighlight::spansForLine(int row) const
{
	if (row < 0 || row >= static_cast<int>(lineColors.size()))
		return kEmptySpans;
	return lineColors[static_cast<size_t>(row)];
}

// ---------------------------------------------------------------------------
// Span morph tracks insert/delete geometry from TextOp only.
// lineLens is private geometry for multi-line deletes (no ops coupling).
// ---------------------------------------------------------------------------

void EditorHighlight::insertBytes(LineColorSpans &spans, int col, int n)
{
	if (n <= 0)
		return;
	for (ColorSpan &s : spans)
	{
		if (s.start >= col)
		{
			s.start += n;
			s.end += n;
		} else if (s.end > col)
		{
			s.end += n;
		}
	}
}

void EditorHighlight::deleteBytes(LineColorSpans &spans, int col, int n)
{
	if (n <= 0)
		return;
	const int delEnd = col + n;
	LineColorSpans out;
	out.reserve(spans.size());
	for (const ColorSpan &s : spans)
	{
		if (s.end <= col)
			out.push_back(s);
		else if (s.start >= delEnd)
			out.push_back({s.start - n, s.end - n, s.color});
		else
		{
			if (s.start < col)
				out.push_back({s.start, col, s.color});
			if (s.end > delEnd)
				out.push_back({col, s.end - n, s.color});
		}
	}
	out.erase(
		std::remove_if(
			out.begin(), out.end(), [](const ColorSpan &s) { return s.start >= s.end; }),
		out.end());
	spans = std::move(out);
}

void EditorHighlight::morphInsert(int row, int col, const std::string &text)
{
	if (text.empty() || row < 0 || !state)
		return;
	if (static_cast<size_t>(row) >= lineColors.size())
	{
		lineColors.resize(static_cast<size_t>(row) + 1);
		lineLens.resize(static_cast<size_t>(row) + 1, 0);
	}

	const auto parts = EditorUtils::SplitOnSeparator(text, state->lineEnding);
	if (parts.size() == 1)
	{
		const int n = static_cast<int>(parts[0].size());
		insertBytes(lineColors[static_cast<size_t>(row)], col, n);
		lineLens[static_cast<size_t>(row)] += n;
		return;
	}

	LineColorSpans &cur = lineColors[static_cast<size_t>(row)];
	LineColorSpans head, tail;
	for (const ColorSpan &s : cur)
	{
		if (s.end <= col)
			head.push_back(s);
		else if (s.start >= col)
			tail.push_back({s.start - col, s.end - col, s.color});
		else
		{
			head.push_back({s.start, col, s.color});
			tail.push_back({0, s.end - col, s.color});
		}
	}

	const int firstLen = static_cast<int>(parts[0].size());
	const int lastLen = static_cast<int>(parts.back().size());
	const int oldLen = lineLens[static_cast<size_t>(row)];
	const int tailLen = std::max(0, oldLen - col);

	insertBytes(head, col, firstLen);
	cur = std::move(head);
	lineLens[static_cast<size_t>(row)] = col + firstLen;

	std::vector<LineColorSpans> added(parts.size() - 1);
	std::vector<int> addedLens(parts.size() - 1);
	for (size_t i = 0; i + 1 < parts.size(); ++i)
		addedLens[i] = static_cast<int>(parts[i + 1].size());
	// Last new line carries the old tail after the inserted suffix.
	addedLens.back() = lastLen + tailLen;
	for (ColorSpan &s : tail)
	{
		s.start += lastLen;
		s.end += lastLen;
	}
	added.back() = std::move(tail);

	lineColors.insert(lineColors.begin() + row + 1, added.begin(), added.end());
	lineLens.insert(lineLens.begin() + row + 1, addedLens.begin(), addedLens.end());
}

void EditorHighlight::morphDelete(int row, int col, int length)
{
	// Multi-line delete on spans + lineLens (mirrors document delete shape).
	if (length <= 0 || lineColors.empty() || !state)
		return;

	row = std::clamp(row, 0, static_cast<int>(lineColors.size()) - 1);
	col = std::clamp(col, 0, lineLens[static_cast<size_t>(row)]);
	int remaining = length;
	const int sepLen = static_cast<int>(state->lineEnding.size());

	while (remaining > 0 && row < static_cast<int>(lineColors.size()))
	{
		const int lineLen = lineLens[static_cast<size_t>(row)];
		const int avail = lineLen - col;

		if (remaining <= avail)
		{
			deleteBytes(lineColors[static_cast<size_t>(row)], col, remaining);
			lineLens[static_cast<size_t>(row)] -= remaining;
			return;
		}

		// Delete through end of this line.
		deleteBytes(lineColors[static_cast<size_t>(row)], col, 1 << 30);
		remaining -= avail;
		lineLens[static_cast<size_t>(row)] = col;

		if (row + 1 >= static_cast<int>(lineColors.size()))
			return;

		// Consume the line-ending between this line and the next.
		if (remaining < sepLen)
			remaining = 0;
		else
			remaining -= sepLen;

		// Merge next line's spans onto (row, col), then drop the next row.
		LineColorSpans next = std::move(lineColors[static_cast<size_t>(row + 1)]);
		const int nextLen = lineLens[static_cast<size_t>(row + 1)];
		for (ColorSpan &s : next)
		{
			s.start += col;
			s.end += col;
		}
		auto &dest = lineColors[static_cast<size_t>(row)];
		dest.insert(dest.end(), next.begin(), next.end());
		lineLens[static_cast<size_t>(row)] = col + nextLen;

		lineColors.erase(lineColors.begin() + row + 1);
		lineLens.erase(lineLens.begin() + row + 1);
		// Continue from same (row, col) into the merged text.
	}
}

void EditorHighlight::morphSpans(const std::vector<PendingTreeEdit> &edits)
{
	if (!state)
		return;

	// lineLens must describe pre-edit geometry (kept in sync by prior morph/publish).
	// Content is already post-edit, so never rebuild lens from content here.
	if (lineLens.size() != lineColors.size() || lineColors.empty())
	{
		lineColors.assign(static_cast<size_t>(state->lineCount()), {});
		syncLensFromContent();
		return;
	}

	for (const PendingTreeEdit &pe : edits)
	{
		if (pe.op.kind == OpKind::Insert)
			morphInsert(pe.op.row, pe.op.column, pe.op.text);
		else
			morphDelete(pe.op.row, pe.op.column, pe.op.length);
	}
}

// ---------------------------------------------------------------------------
// Snapshot / recolor / publish
// ---------------------------------------------------------------------------

size_t EditorHighlight::contentByteSize() const
{
	if (!state)
		return 0;
	return state->byteSize();
}

TreeSitter::ParseSnapshot
EditorHighlight::makeSnapshot(std::vector<PendingTreeEdit> pending) const
{
	TreeSitter::ParseSnapshot snap;
	if (!state)
		return snap;
	// CoW snapshot only — cheap. lineStarts are filled in TreeSitter::parse
	// (UI thread for small files, worker for large ones).
	snap.text = state->snapshot();
	snap.lineEnding = state->lineEnding;
	snap.path = state->path;
	snap.languageId = state->languageId;
	snap.pendingEdits = std::move(pending);
	return snap;
}

void EditorHighlight::recolor()
{
	if (!state || !operations)
		return;

	const size_t totalBytes = contentByteSize();
	const bool useTreeSitter = settings && settings->settings.value("treesitter", true);
	if (!useTreeSitter || totalBytes > kSkipTreeSitterBytes)
	{
		heldTreeEdits.clear();
		return;
	}

	cancelHighlighting();
	const uint64_t gen = operations->generation();
	jobGen = gen;
	auto pending = std::move(heldTreeEdits);
	heldTreeEdits.clear();

	TreeSitter::ParseSnapshot snap = makeSnapshot(std::move(pending));
	// O(1) from rope summary — do not walk lineStarts on the UI thread.
	const size_t lineCount = static_cast<size_t>(std::max(1, snap.text.lineCount()));

	// Small files: recolor on this thread (same frame gets real colors).
	// lineStarts walk happens inside parse() here — fine for small docs.
	if (totalBytes <= kAsyncRecolorBytes)
	{
		try
		{
			ParseResult built = treeSitter.parse(std::move(snap), gen);
			if (built.ok && built.lineCount == lineCount &&
				operations->generation() == gen)
				applyParseResult(built);
		} catch (const std::exception &)
		{
		}
		return;
	}

	// Large files: morph already updated spans on the UI thread (instant).
	// lineStarts + tree-sitter run on a worker so typing stays responsive.
	auto canceled = std::make_shared<std::atomic_bool>(false);
	cancelFlag = canceled;
	tasks.push_back(std::async(
		std::launch::async,
		[this, canceled, snap = std::move(snap), gen, lineCount]() mutable {
			if (canceled->load())
				return;
			try
			{
				ParseResult built = treeSitter.parse(std::move(snap), gen);
				if (canceled->load() || !built.ok || built.lineCount != lineCount)
					return;
				auto ptr = std::make_shared<ParseResult>(std::move(built));
				std::lock_guard lock(pendingMutex);
				if (pendingResult && pendingGen > gen)
					return;
				pendingResult = std::move(ptr);
				pendingGen = gen;
			} catch (const std::exception &)
			{
			}
		}));
}

void EditorHighlight::applyParseResult(ParseResult &result)
{
	if (!result.ok)
		return;

	if (result.full)
	{
		if (result.fullColors.size() != result.lineCount)
			return;
		lineColors = std::move(result.fullColors);
		// Full rebuild: refresh lineLens from the buffer.
		syncLensFromContent();
	} else
	{
		// Partial: morph already sized maps; only replace dirty rows.
		if (lineColors.size() != result.lineCount)
			lineColors.assign(result.lineCount, {});
		const size_t n = std::min(result.dirtyRows.size(), result.dirtySpans.size());
		for (size_t i = 0; i < n; ++i)
		{
			const int row = result.dirtyRows[i];
			if (row < 0 || row >= static_cast<int>(lineColors.size()))
				continue;
			lineColors[static_cast<size_t>(row)] = std::move(result.dirtySpans[i]);
		}
	}
}

void EditorHighlight::publishPending()
{
	std::shared_ptr<ParseResult> ready;
	uint64_t gen = 0;
	{
		std::lock_guard lock(pendingMutex);
		if (!pendingResult || pendingGen < jobGen)
		{
			pendingResult.reset();
			return;
		}
		ready = std::move(pendingResult);
		gen = pendingGen;
	}
	if (!ready || (operations && operations->generation() != gen))
		return;
	applyParseResult(*ready);
}

void EditorHighlight::poll()
{
	reapTasks();
	publishPending();
}

void EditorHighlight::highlightContent()
{
	if (!state)
		return;

	// Keep theme colors current (tabs / theme switches).
	treeSitter.updateThemeColors();

	std::vector<PendingTreeEdit> pending;
	if (operations)
		pending = operations->takePending();

	if (!pending.empty())
	{
		morphSpans(pending);
		heldTreeEdits.insert(heldTreeEdits.end(),
							 std::make_move_iterator(pending.begin()),
							 std::make_move_iterator(pending.end()));
	} else if (static_cast<int>(lineColors.size()) != state->lineCount())
	{
		lineColors.assign(static_cast<size_t>(state->lineCount()), {});
		syncLensFromContent();
		heldTreeEdits.clear();
	}

	recolor();
}
