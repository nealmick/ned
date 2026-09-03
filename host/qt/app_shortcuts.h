/*
	File: host/qt/app_shortcuts.h
	Description: Global app shortcuts for the Qt host (tab switching,
	splits, terminal, find, line jump, file finder, settings, open).
	Counterpart of host/imgui/app_shortcuts.cpp (keybind polling).
*/

#pragma once

class AppHost;

// Installs every host-level QShortcut on the host window.
void installAppShortcuts(AppHost &host);
