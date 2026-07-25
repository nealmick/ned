#include "project_undo.h"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

namespace {
const char *UNDO_FILE_NAME = ".undo-redo-ned.json";

SelectionSnapshot collapsedSnap(int row, int column)
{
	SelectionSnapshot s;
	s.headRow = s.anchorRow = row;
	s.headColumn = s.anchorColumn = column;
	return s;
}

nlohmann::json stepToJson(const HistoryStep &step)
{
	return {{"kind", step.op.kind == OpKind::Insert ? "insert" : "delete"},
			{"row", step.op.row},
			{"column", step.op.column},
			{"text", step.op.text},
			{"length", step.op.length},
			{"deletedText", step.deletedText}};
}

HistoryStep stepFromJson(const nlohmann::json &item)
{
	HistoryStep step;
	const std::string kind = item.value("kind", "insert");
	step.op.kind = (kind == "delete") ? OpKind::Delete : OpKind::Insert;
	step.op.row = item.value("row", 0);
	step.op.column = item.value("column", 0);
	step.op.text = item.value("text", "");
	step.op.length = item.value("length", 0);
	step.deletedText = item.value("deletedText", "");
	return step;
}

nlohmann::json snapToJson(const SelectionSnapshot &s)
{
	return {{"headRow", s.headRow},
			{"headColumn", s.headColumn},
			{"anchorRow", s.anchorRow},
			{"anchorColumn", s.anchorColumn},
			{"preferredColumn", s.preferredColumn}};
}

SelectionSnapshot snapFromJson(const nlohmann::json &item)
{
	SelectionSnapshot s;
	s.headRow = item.value("headRow", 0);
	s.headColumn = item.value("headColumn", 0);
	s.anchorRow = item.value("anchorRow", s.headRow);
	s.anchorColumn = item.value("anchorColumn", s.headColumn);
	s.preferredColumn = item.value("preferredColumn", 0);
	return s;
}

void loadSnapArray(const nlohmann::json &arr, std::vector<SelectionSnapshot> &out)
{
	out.clear();
	if (!arr.is_array())
		return;
	for (const auto &item : arr)
	{
		if (item.is_object())
			out.push_back(snapFromJson(item));
	}
}
} // namespace

// ---------------------------------------------------------------------------
// FileStack
// ---------------------------------------------------------------------------

void ProjectUndo::FileStack::initialize()
{
	hasPending = false;
	undoStack.clear();
	redoStack.clear();
}

bool ProjectUndo::FileStack::tryCoalesce(const RecordedEdit &edit)
{
	// Only single-step adjacent inserts on one caret.
	if (!hasPending || pending.steps.size() != 1 || edit.steps.size() != 1)
		return false;
	if (pending.selectionsAfter.size() != 1 || edit.selectionsAfter.size() != 1)
		return false;

	const HistoryStep &prev = pending.steps[0];
	const HistoryStep &next = edit.steps[0];
	if (prev.op.kind != OpKind::Insert || next.op.kind != OpKind::Insert)
		return false;

	const int expectCol = prev.op.column + static_cast<int>(prev.op.text.size());
	if (next.op.row != prev.op.row || next.op.column != expectCol)
		return false;

	pending.steps[0].op.text += next.op.text;
	pending.selectionsAfter = edit.selectionsAfter;
	pending.primaryAfter = edit.primaryAfter;
	lastAddTime = std::chrono::steady_clock::now();
	return true;
}

bool ProjectUndo::FileStack::pendingExpired() const
{
	if (!hasPending)
		return false;
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
							 std::chrono::steady_clock::now() - lastAddTime)
							 .count();
	return elapsed >= COALESCE_MS;
}

void ProjectUndo::FileStack::record(RecordedEdit edit)
{
	if (edit.steps.empty())
		return;

	redoStack.clear();

	if (hasPending && pendingExpired())
		commitPending();

	if (hasPending && tryCoalesce(edit))
		return;

	if (hasPending)
		commitPending();

	pending = std::move(edit);
	hasPending = true;
	lastAddTime = std::chrono::steady_clock::now();
}

void ProjectUndo::FileStack::commitPending()
{
	if (!hasPending)
		return;
	undoStack.push_back(std::move(pending));
	if (undoStack.size() > MAX_STACK)
		undoStack.erase(undoStack.begin());
	hasPending = false;
}

void ProjectUndo::FileStack::flushPending() { commitPending(); }

std::pair<ProjectUndo::RecordedEdit, bool> ProjectUndo::FileStack::undo()
{
	commitPending();
	if (undoStack.empty())
		return {{}, false};
	RecordedEdit edit = undoStack.back();
	undoStack.pop_back();
	redoStack.push_back(edit);
	return {edit, true};
}

std::pair<ProjectUndo::RecordedEdit, bool> ProjectUndo::FileStack::redo()
{
	commitPending();
	if (redoStack.empty())
		return {{}, false};
	RecordedEdit edit = redoStack.back();
	redoStack.pop_back();
	undoStack.push_back(edit);
	return {edit, true};
}

void ProjectUndo::FileStack::updatePendingFinalCursor(int row, int column)
{
	if (!hasPending || pending.selectionsAfter.empty())
		return;
	int pi = pending.primaryAfter;
	if (pi < 0 || pi >= static_cast<int>(pending.selectionsAfter.size()))
		pi = 0;
	SelectionSnapshot &s = pending.selectionsAfter[static_cast<size_t>(pi)];
	s.headRow = row;
	s.headColumn = column;
}

bool ProjectUndo::FileStack::hasOperations() const
{
	return hasPending || !undoStack.empty() || !redoStack.empty();
}

nlohmann::json ProjectUndo::FileStack::toJson() const
{
	nlohmann::json j;
	j["version"] = 3;
	j["undoStack"] = nlohmann::json::array();
	j["redoStack"] = nlohmann::json::array();

	auto push = [](nlohmann::json &arr, const RecordedEdit &e) {
		nlohmann::json item;
		item["steps"] = nlohmann::json::array();
		for (const HistoryStep &step : e.steps)
			item["steps"].push_back(stepToJson(step));
		item["selectionsBefore"] = nlohmann::json::array();
		for (const SelectionSnapshot &s : e.selectionsBefore)
			item["selectionsBefore"].push_back(snapToJson(s));
		item["selectionsAfter"] = nlohmann::json::array();
		for (const SelectionSnapshot &s : e.selectionsAfter)
			item["selectionsAfter"].push_back(snapToJson(s));
		item["primaryBefore"] = e.primaryBefore;
		item["primaryAfter"] = e.primaryAfter;
		arr.push_back(std::move(item));
	};

	if (hasPending)
		push(j["undoStack"], pending);

	const size_t undoCount = std::min(undoStack.size(), size_t{50});
	for (size_t i = undoStack.size() - undoCount; i < undoStack.size(); ++i)
		push(j["undoStack"], undoStack[i]);
	const size_t redoCount = std::min(redoStack.size(), size_t{20});
	for (size_t i = redoStack.size() - redoCount; i < redoStack.size(); ++i)
		push(j["redoStack"], redoStack[i]);
	return j;
}

void ProjectUndo::FileStack::fromJson(const nlohmann::json &j)
{
	try
	{
		if (!j.contains("version"))
		{
			undoStack.clear();
			redoStack.clear();
			return;
		}
		const int version = j["version"].get<int>();
		if (version < 2)
		{
			undoStack.clear();
			redoStack.clear();
			return;
		}

		undoStack.clear();
		redoStack.clear();
		hasPending = false;

		auto loadV2Item = [](const nlohmann::json &item, RecordedEdit &e) {
			HistoryStep step = stepFromJson(item);
			// v2 stored deletedText on the same object as the op.
			if (item.contains("deletedText"))
				step.deletedText = item.value("deletedText", "");
			e.steps = {std::move(step)};
			const int br = item.value("cursorBeforeRow", 0);
			const int bc = item.value("cursorBeforeColumn", 0);
			const int ar = item.value("cursorAfterRow", 0);
			const int ac = item.value("cursorAfterColumn", 0);
			e.selectionsBefore = {collapsedSnap(br, bc)};
			e.selectionsAfter = {collapsedSnap(ar, ac)};
			e.primaryBefore = 0;
			e.primaryAfter = 0;
		};

		auto loadV3Item = [](const nlohmann::json &item, RecordedEdit &e) {
			e.steps.clear();
			if (item.contains("steps") && item["steps"].is_array())
			{
				for (const auto &s : item["steps"])
				{
					if (s.is_object())
						e.steps.push_back(stepFromJson(s));
				}
			} else
			{
				// Single-step object without steps array.
				e.steps.push_back(stepFromJson(item));
			}
			if (item.contains("selectionsBefore"))
				loadSnapArray(item["selectionsBefore"], e.selectionsBefore);
			if (item.contains("selectionsAfter"))
				loadSnapArray(item["selectionsAfter"], e.selectionsAfter);
			e.primaryBefore = item.value("primaryBefore", 0);
			e.primaryAfter = item.value("primaryAfter", 0);
			if (e.selectionsBefore.empty())
				e.selectionsBefore.push_back(SelectionSnapshot{});
			if (e.selectionsAfter.empty())
				e.selectionsAfter.push_back(SelectionSnapshot{});
		};

		auto load = [&](const nlohmann::json &arr, std::vector<RecordedEdit> &out) {
			if (!arr.is_array())
				return;
			for (const auto &item : arr)
			{
				if (!item.is_object())
					continue;
				RecordedEdit e;
				if (version >= 3 && item.contains("steps"))
					loadV3Item(item, e);
				else
					loadV2Item(item, e);
				if (!e.steps.empty())
					out.push_back(std::move(e));
			}
		};

		if (j.contains("undoStack"))
			load(j["undoStack"], undoStack);
		if (j.contains("redoStack"))
			load(j["redoStack"], redoStack);
	} catch (const std::exception &e)
	{
		std::cerr << "Error loading undo stack: " << e.what() << '\n';
		undoStack.clear();
		redoStack.clear();
		hasPending = false;
	}
}

// ---------------------------------------------------------------------------
// ProjectUndo
// ---------------------------------------------------------------------------

ProjectUndo::FileStack *ProjectUndo::stackFor(const std::string &path)
{
	if (path.empty())
		return nullptr;
	auto it = stacks.find(path);
	if (it == stacks.end())
	{
		it = stacks.emplace(path, FileStack()).first;
		it->second.initialize();
	}
	return &it->second;
}

void ProjectUndo::ensureFile(const std::string &path)
{
	if (path.empty())
		return;
	if (auto *s = stackFor(path))
		s->flushPending();
	maybeSaveToDisk();
}

void ProjectUndo::record(const std::string &path, HistoryEdit edit)
{
	auto *s = stackFor(path);
	if (!s || edit.steps.empty())
		return;

	RecordedEdit rec;
	rec.steps = std::move(edit.steps);
	rec.selectionsBefore = std::move(edit.selectionsBefore);
	rec.selectionsAfter = std::move(edit.selectionsAfter);
	rec.primaryBefore = edit.primaryBefore;
	rec.primaryAfter = edit.primaryAfter;
	if (rec.selectionsBefore.empty())
		rec.selectionsBefore.push_back(SelectionSnapshot{});
	if (rec.selectionsAfter.empty())
		rec.selectionsAfter.push_back(SelectionSnapshot{});
	s->record(std::move(rec));
	dirty = true;
	maybeSaveToDisk();
}

void ProjectUndo::record(const std::string &path,
						 const TextOp &op,
						 const std::string &deletedText,
						 int cursorRowBefore,
						 int cursorColumnBefore,
						 int cursorRowAfter,
						 int cursorColumnAfter)
{
	HistoryEdit edit;
	edit.steps.push_back({op, deletedText});
	edit.selectionsBefore = {collapsedSnap(cursorRowBefore, cursorColumnBefore)};
	edit.selectionsAfter = {collapsedSnap(cursorRowAfter, cursorColumnAfter)};
	record(path, std::move(edit));
}

void ProjectUndo::updatePendingCursor(const std::string &path, int row, int column)
{
	if (auto *s = stackFor(path))
		s->updatePendingFinalCursor(row, column);
}

bool ProjectUndo::undo(const std::string &path, HistoryEdit &out)
{
	auto *s = stackFor(path);
	if (!s)
		return false;
	auto [edit, valid] = s->undo();
	if (!valid)
		return false;
	out = edit.toHistory();
	dirty = true;
	maybeSaveToDisk();
	return true;
}

bool ProjectUndo::redo(const std::string &path, HistoryEdit &out)
{
	auto *s = stackFor(path);
	if (!s)
		return false;
	auto [edit, valid] = s->redo();
	if (!valid)
		return false;
	out = edit.toHistory();
	dirty = true;
	maybeSaveToDisk();
	return true;
}

void ProjectUndo::flush()
{
	if (!dirty || !projectRoot || projectRoot->empty())
		return;
	saveProject(*projectRoot);
}

void ProjectUndo::maybeSaveToDisk()
{
	if (!dirty || !projectRoot || projectRoot->empty())
		return;

	const auto now = std::chrono::steady_clock::now();
	const auto elapsed =
		std::chrono::duration_cast<std::chrono::milliseconds>(now - lastDiskSave).count();
	if (elapsed < DISK_SAVE_INTERVAL_MS && lastDiskSave.time_since_epoch().count() != 0)
		return;

	saveProject(*projectRoot);
	lastDiskSave = now;
}

void ProjectUndo::saveProject(const std::string &folder)
{
	if (folder.empty() || !dirty)
		return;

	const fs::path undoPath = fs::path(folder) / UNDO_FILE_NAME;
	try
	{
		nlohmann::json root;
		bool hasChanges = false;

		// Full project map — safe for multi-tab: one store owns every path.
		for (auto &[path, stack] : stacks)
		{
			stack.flushPending();
			if (!stack.hasOperations())
				continue;
			try
			{
				root["files"][path] = stack.toJson();
				hasChanges = true;
			} catch (const std::exception &e)
			{
				std::cerr << "Error serializing undo for " << path << ": " << e.what()
						  << '\n';
			}
		}

		if (hasChanges)
		{
			std::ofstream file(undoPath);
			if (file)
				file << root.dump(4);
		}
		dirty = false;
	} catch (const std::exception &e)
	{
		std::cerr << "Error saving undo state: " << e.what() << '\n';
	}
}

void ProjectUndo::loadProject(const std::string &folder)
{
	if (folder.empty())
		return;

	// Drop in-memory history for the previous project; disk is source of truth.
	stacks.clear();
	dirty = false;

	const fs::path undoPath = fs::path(folder) / UNDO_FILE_NAME;
	std::ifstream file(undoPath);
	if (!file)
		return;

	try
	{
		nlohmann::json root;
		file >> root;
		if (!root.contains("files") || !root["files"].is_object())
			return;

		for (auto &[key, value] : root["files"].items())
		{
			try
			{
				stacks[key].fromJson(value);
			} catch (const std::exception &e)
			{
				std::cerr << "Error loading undo for " << key << ": " << e.what() << '\n';
			}
		}
	} catch (const std::exception &e)
	{
		std::cerr << "Error loading undo state: " << e.what() << '\n';
		std::error_code ec;
		fs::remove(undoPath, ec);
		stacks.clear();
	}
}
