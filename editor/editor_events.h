/*
	File: editor_events.h
	Description: Editor change notifications. Owned by Editor; services subscribe.
	Emitters do not know who listens.
*/

#pragma once

#include <functional>
#include <string>
#include <vector>

class EditorEvents
{
  public:
	// One sequential document mutation (UTF-16 columns, LSP-style).
	// Applied in order: insert has start==end; delete has empty text.
	struct DocumentChange
	{
		int startLine = 0;
		int startCharacter = 0;
		int endLine = 0;
		int endCharacter = 0;
		std::string text;
	};

	struct DidEdit
	{
		int version = 0;
		int firstRow = 0; // inclusive post-edit dirty span (view caches)
		int lastRow = 0;
		std::vector<DocumentChange> changes;
	};

	struct DidSave
	{
		std::string path;
		int version = 0;
	};

	// Mutual exclusion for overlays (editor + shell). Composition root closes
	// settings / file-finder; editor dismisses line-jump / find as needed.
	// keep == None means close every overlay.
	struct DidRequestExclusiveOverlay
	{
		enum class Keep { None, Settings, LineJump, FileFinder, Find };
		Keep keep = Keep::None;
	};

	using DidEditFn = std::function<void(const DidEdit &)>;
	using DidSaveFn = std::function<void(const DidSave &)>;
	using DidRequestExclusiveOverlayFn =
		std::function<void(const DidRequestExclusiveOverlay &)>;

	void subscribeDidEdit(DidEditFn fn);
	void subscribeDidSave(DidSaveFn fn);
	void subscribeDidRequestExclusiveOverlay(DidRequestExclusiveOverlayFn fn);

	void emitDidEdit(const DidEdit &e);
	void emitDidSave(const DidSave &e);
	void emitDidRequestExclusiveOverlay(const DidRequestExclusiveOverlay &e);

	void clear();

  private:
	std::vector<DidEditFn> didEditListeners;
	std::vector<DidSaveFn> didSaveListeners;
	std::vector<DidRequestExclusiveOverlayFn> exclusiveOverlayListeners;
};
