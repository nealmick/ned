/*
	File: views/imgui/settings_view.h
	Description: ImGui settings window, theme styling and notification toast.
	Pure rendering over util/settings.h state — no persistence here.
*/

#pragma once

#include "../../../util/settings.h"

class EditorApi;
class FileExplorer;
class LspImGuiView;
struct ImGuiStyle;

class SettingsView
{
  public:
	SettingsView(Settings &settings) : s(settings) {}

	// Reapply fonts/theme (fonts, style, editor colors). Returns true when
	// fonts were reloaded (atlas may have been rebuilt).
	bool apply(bool force, EditorApi &api);
	void ApplySettings(ImGuiStyle &style);

	void renderSettingsWindow(EditorApi &api, FileExplorer &files, LspImGuiView &lsp);
	void renderNotification();

  private:
	Settings &s;

	void renderSettingsContent(EditorApi &api, FileExplorer &files, LspImGuiView &lsp);
	void renderWindowHeader(EditorApi &api, FileExplorer &files);
	void renderProfileSelector();
	void renderMainSettings();
	void renderMacSettings();
	void renderSyntaxColors();
	void renderToggleSettings();
	void renderShaderSettings();
	void renderShaderSlider(const char *label,
							const char *key,
							float min_val,
							float max_val,
							const char *format,
							float default_val);
	void renderKeybindsSettings(FileExplorer &files, LspImGuiView &lsp);
	void handleWindowInput(EditorApi &api);
	void applyImGuiStyles();
	void closeSettingsWindow(EditorApi &api);

	static std::string displayFontName(const std::string &fontFile);
};
