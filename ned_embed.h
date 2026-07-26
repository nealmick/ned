/*
	File: ned_embed.h
	Description: Thin embed facade — Workbench in Floating (moveable) mode.
*/

#pragma once

#include "workbench.h"

class NedEmbed
{
  public:
	NedEmbed();
	~NedEmbed();

	bool initialize();
	void render();
	void applySettingsChanges();
	void cleanup();

	// Shared workbench (hosts may read settings / active editor via these).
	Workbench workbench;

	Editor *activeEditor() { return workbench.activeEditor(); }
	EditorApi *activeApi() { return workbench.activeApi(); }
};
