/*
	File: services/save_service.h
	Description: Document save — write dirty buffer to disk and emit did-save.
	save() is immediate; onDidEdit schedules; poll() writes after idle.

	Named EditorSave to match EditorGit / EditorHighlight.
*/

#pragma once

#include <chrono>

class EditorState;
class EditorEvents;

class EditorSave
{
  public:
	// Idle debounce before an autosave flush. 500ms: everything downstream
	// of a write (disk watchers, diff refresh, git status) trails this, so
	// it defines how "live" the app feels; still long enough that burst
	// typing coalesces into one write per pause.
	static constexpr int kAutosaveIdleMs = 500;

	EditorSave(EditorState &document, EditorEvents &ev) : state(&document), events(&ev) {}

	void save();					// immediate write if dirty; clears pending autosave
	void onDidEdit();				// schedule autosave (no write)
	void poll();					// frame tick: flush if idle elapsed
	void setAutosaveIdleMs(int ms); // tests

  private:
	EditorState *state;
	EditorEvents *events;
	bool autosavePending = false;
	std::chrono::steady_clock::time_point lastEditTime{};
	int autosaveIdleMs = kAutosaveIdleMs;
};
