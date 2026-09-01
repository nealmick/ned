/*
	File: util/settings_app.cpp
	Description: Settings methods that talk to EditorApi (overlay/input
	muting). Defined outside ned_core so the core library does not link
	against the editor presenter.
*/

#include "settings.h"

#include "../editor/editor_api.h"
#include "../editor/editor_events.h"

void Settings::toggleSettingsWindow(EditorApi &api)
{
	showSettingsWindow = !showSettingsWindow;
	if (showSettingsWindow)
		api.requestExclusiveOverlay(
			EditorEvents::DidRequestExclusiveOverlay::Keep::Settings);
	api.setBlockInput(showSettingsWindow);
}
