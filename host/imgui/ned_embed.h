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

	// Shell-owned settings trio (Qt AppHost parity) + shared workbench.
	Settings settings;
	Font font;
	SettingsView settingsView{settings, font};
	Workbench workbench{settings, font, settingsView};

	Editor *activeView() { return workbench.activeView(); }
	EditorApi *activeApi() { return workbench.activeApi(); }
};
