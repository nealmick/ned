#include "highlight_service.h"
#include "../../../util/settings.h"
#include "../../editor_operations.h"
#include "../../editor_state.h"

#include <algorithm>
#include <iterator>

EditorHighlight::EditorHighlight(EditorState &document,
								 EditorOperations &ops,
								 Settings *appSettings)
	: state(&document), operations(&ops), settings(appSettings)
{
	if (appSettings)
		treeSitter.bind(*appSettings);
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
	treeSitter.updateThemeColors();
	++visualGen_;
	if (!state || state->path.empty())
		return;
	const bool ts = !settings || settings->settings.value("treesitter", true);
	if (!ts)
		return;
	for (const auto &line : spans.lines)
	{
		if (!line.empty())
			return;
	}
	highlightContent();
}

ImVec4 EditorHighlight::defaultTextColor() const
{
	return treeSitter.cachedColors[ThemeSlot::Text];
}

ImVec4 EditorHighlight::colorForSlot(ThemeSlot slot) const
{
	return treeSitter.cachedColors[slot];
}

void EditorHighlight::syncLensFromContent()
{
	spans.lens.clear();
	if (!state)
		return;
	const int n = state->lineCount();
	spans.lens.reserve(static_cast<size_t>(n));
	for (int i = 0; i < n; ++i)
		spans.lens.push_back(state->lineLength(i));
}

void EditorHighlight::clear()
{
	cancelHighlighting();
	++jobGen;
	++visualGen_;
	heldTreeEdits.clear();
	spans.clear();
	std::lock_guard lock(pendingMutex);
	pendingResult.reset();
	pendingGen = 0;
}

void EditorHighlight::resetForDocument(size_t lineCount)
{
	cancelHighlighting();
	++jobGen;
	++visualGen_;
	heldTreeEdits.clear();
	spans.assignEmpty(lineCount);
	syncLensFromContent();
	if (spans.lens.size() != lineCount)
		spans.lens.assign(lineCount, 0);
	std::lock_guard lock(pendingMutex);
	pendingResult.reset();
	pendingGen = 0;
}

const LineColorSpans &EditorHighlight::spansForLine(int row) const
{
	return spans.at(row);
}

void EditorHighlight::morphSpans(const std::vector<PendingTreeEdit> &edits)
{
	if (!state)
		return;
	if (spans.applyEdits(edits, state->lineEnding))
	{
		++visualGen_;
		return;
	}
	spans.lines.assign(static_cast<size_t>(state->lineCount()), {});
	syncLensFromContent();
	++visualGen_;
}

TreeSitter::ParseSnapshot
EditorHighlight::makeSnapshot(std::vector<PendingTreeEdit> pending) const
{
	TreeSitter::ParseSnapshot snap;
	if (!state)
		return snap;
	snap.text = state->snapshot();
	snap.lineEnding = state->lineEnding;
	snap.path = state->path;
	snap.languageId = state->languageId;
	snap.pendingEdits = std::move(pending);
	return snap;
}

void EditorHighlight::postResult(ParseResult result, uint64_t gen)
{
	if (result.kind == ParseKind::Failed || result.kind == ParseKind::TreeOnly)
		return;
	std::lock_guard lock(pendingMutex);
	if (pendingResult && pendingGen > gen)
		return;
	if (pendingResult && pendingGen == gen && pendingResult->kind == ParseKind::Partial &&
		result.kind == ParseKind::Partial)
	{
		pendingResult->dirtyRows.insert(pendingResult->dirtyRows.end(),
										result.dirtyRows.begin(),
										result.dirtyRows.end());
		pendingResult->dirtySpans.insert(
			pendingResult->dirtySpans.end(),
			std::make_move_iterator(result.dirtySpans.begin()),
			std::make_move_iterator(result.dirtySpans.end()));
		return;
	}
	pendingResult = std::make_shared<ParseResult>(std::move(result));
	pendingGen = gen;
}

void EditorHighlight::runJob(TreeSitter::ParseSnapshot &snap,
							 uint64_t gen,
							 size_t lineCount,
							 const std::shared_ptr<std::atomic_bool> &canceled)
{
	treeSitter.colorDocument(
		snap,
		gen,
		kQueryChunkLines,
		[&]() { return canceled && canceled->load(); },
		[&](ParseResult &&built) {
			if (built.kind != ParseKind::Failed && built.lineCount == lineCount)
				postResult(std::move(built), gen);
		});
}

void EditorHighlight::launchJob(TreeSitter::ParseSnapshot snap,
								uint64_t gen,
								size_t lineCount)
{
	auto canceled = std::make_shared<std::atomic_bool>(false);
	cancelFlag = canceled;
	tasks.push_back(
		std::async(std::launch::async,
				   [this, canceled, snap = std::move(snap), gen, lineCount]() mutable {
					   if (canceled->load())
						   return;
					   try
					   {
						   runJob(snap, gen, lineCount, canceled);
					   } catch (const std::exception &)
					   {
					   }
				   }));
}

void EditorHighlight::recolor()
{
	if (!state || !operations)
		return;

	const size_t bytes = state->byteSize();
	const bool useTreeSitter = !settings || settings->settings.value("treesitter", true);
	if (!useTreeSitter || bytes > kSkipTreeSitterBytes)
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
	const size_t lineCount = static_cast<size_t>(std::max(1, snap.text.lineCount()));
	const bool incremental = !snap.pendingEdits.empty();
	const bool tiny = bytes <= kSyncIncrementalBytes;
	const bool ready = treeSitter.queryReady(snap.languageId);

	// Query already compiled: small docs color on this thread (one parse).
	// Cold compile stays on the worker so opening a 10-line file cannot hitch.
	if (tiny && ready)
	{
		try
		{
			runJob(snap, gen, lineCount, nullptr);
			publishPending();
		} catch (const std::exception &)
		{
		}
		return;
	}

	if (!incremental && ready)
	{
		try
		{
			ParseResult part = treeSitter.queryPrefix(snap, kPrimeQueryLines);
			applyParseResult(part);
		} catch (const std::exception &)
		{
		}
	}
	launchJob(std::move(snap), gen, lineCount);
}

void EditorHighlight::applyParseResult(ParseResult &result)
{
	if (result.kind == ParseKind::Full)
	{
		if (result.fullColors.size() != result.lineCount)
			return;
		spans.lines = std::move(result.fullColors);
		syncLensFromContent();
		++visualGen_;
		return;
	}
	if (result.kind != ParseKind::Partial)
		return;

	if (spans.lines.size() != result.lineCount)
		spans.lines.assign(result.lineCount, {});
	const size_t n = std::min(result.dirtyRows.size(), result.dirtySpans.size());
	for (size_t i = 0; i < n; ++i)
	{
		const int row = result.dirtyRows[i];
		if (row < 0 || row >= static_cast<int>(spans.lines.size()))
			continue;
		spans.lines[static_cast<size_t>(row)] = std::move(result.dirtySpans[i]);
	}
	if (n > 0)
		++visualGen_;
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
	} else if (static_cast<int>(spans.lines.size()) != state->lineCount())
	{
		spans.lines.assign(static_cast<size_t>(state->lineCount()), {});
		syncLensFromContent();
		heldTreeEdits.clear();
	}

	recolor();
}
