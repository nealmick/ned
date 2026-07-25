/*
	File: util/project_undo.h
	Description: Project-scoped undo/redo store. One instance per app/project;
	all editors share it so multi-tab cannot clobber a single JSON file.

	Stacks are keyed by absolute file path. Editors only apply ops to their
	own buffer — this type owns history + disk, not document mutation.

	Each stack entry is a group of TextOps plus selection snapshots so
	multi-caret edits undo/redo as one step.
*/

#pragma once

#include "../editor/editor_operations.h"
#include "../lib/json.hpp"

#include <chrono>
#include <map>
#include <string>
#include <utility>
#include <vector>

// View-layer selection at a history boundary (no EditorViewState dependency).
struct SelectionSnapshot
{
	int headRow = 0;
	int headColumn = 0;
	int anchorRow = 0;
	int anchorColumn = 0;
	int preferredColumn = 0;
};

struct HistoryStep
{
	TextOp op;
	std::string deletedText;
};

struct HistoryEdit
{
	std::vector<HistoryStep> steps;
	std::vector<SelectionSnapshot> selectionsBefore;
	std::vector<SelectionSnapshot> selectionsAfter;
	int primaryBefore = 0;
	int primaryAfter = 0;
};

class ProjectUndo
{
  public:
	explicit ProjectUndo(std::string &projectRootRef) : projectRoot(&projectRootRef) {}

	// Replace in-memory stacks from <folder>/.undo-redo-ned.json (once per project open).
	void loadProject(const std::string &folder);

	// Ensure a stack exists for path (call when a document is opened).
	void ensureFile(const std::string &path);

	// Record one undo group (one or more steps + selection snapshots).
	void record(const std::string &path, HistoryEdit edit);

	// Convenience: single op + single collapsed caret before/after.
	void record(const std::string &path,
				const TextOp &op,
				const std::string &deletedText,
				int cursorRowBefore,
				int cursorColumnBefore,
				int cursorRowAfter,
				int cursorColumnAfter);

	void updatePendingCursor(const std::string &path, int row, int column);

	// Pop one group; false if nothing to do. Caller applies to the editor buffer.
	bool undo(const std::string &path, HistoryEdit &out);
	bool redo(const std::string &path, HistoryEdit &out);

	// Best-effort write if dirty (e.g. shutdown).
	void flush();

  private:
	const std::string *projectRoot = nullptr;

	struct RecordedEdit
	{
		std::vector<HistoryStep> steps;
		std::vector<SelectionSnapshot> selectionsBefore;
		std::vector<SelectionSnapshot> selectionsAfter;
		int primaryBefore = 0;
		int primaryAfter = 0;

		HistoryEdit toHistory() const
		{
			return {steps, selectionsBefore, selectionsAfter, primaryBefore, primaryAfter};
		}
	};

	class FileStack
	{
	  public:
		void initialize();
		void record(RecordedEdit edit);
		std::pair<RecordedEdit, bool> undo();
		std::pair<RecordedEdit, bool> redo();
		void updatePendingFinalCursor(int row, int column);
		bool hasOperations() const;
		void flushPending();

		nlohmann::json toJson() const;
		void fromJson(const nlohmann::json &j);

	  private:
		static constexpr size_t MAX_STACK = 50;
		static constexpr int COALESCE_MS = 300;

		std::vector<RecordedEdit> undoStack;
		std::vector<RecordedEdit> redoStack;
		RecordedEdit pending;
		bool hasPending = false;
		std::chrono::steady_clock::time_point lastAddTime{};

		void commitPending();
		bool tryCoalesce(const RecordedEdit &edit);
		bool pendingExpired() const;
	};

	std::map<std::string, FileStack> stacks;
	bool dirty = false;
	std::chrono::steady_clock::time_point lastDiskSave{};

	static constexpr int DISK_SAVE_INTERVAL_MS = 3000;

	FileStack *stackFor(const std::string &path);
	void maybeSaveToDisk();
	void saveProject(const std::string &folder);
};
