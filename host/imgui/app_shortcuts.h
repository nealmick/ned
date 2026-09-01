/*
	File: host/imgui/app_shortcuts.h
	Description: Global app keyboard shortcuts, ImGui backend.
*/

#pragma once

class EditorApi;
class FileExplorer;
class Settings;
class LspImGuiView;

bool handleAppKeyboardShortcuts(EditorApi &api,
								FileExplorer &files,
								Settings &settings,
								LspImGuiView &lsp);
