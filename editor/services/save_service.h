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
	static constexpr int kAutosaveIdleMs = 1000;

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
