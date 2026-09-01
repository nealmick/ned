/*
	File: views/imgui/settings_view.cpp
	Description: ImGui settings window and theme application. All rendering
	lives here; util/settings.cpp keeps only persistence and app state.
*/

#include "settings_view.h"
#include "font.h"
#include "../files/files.h"
#include "../lsp/lsp_client.h"
#include "../util/settings.h"
#include "../editor/editor_api.h"
#include "../editor/editor_events.h"
#include "../lsp/views/imgui/lsp_view.h"

#include "imgui.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>

#ifdef __APPLE__
#include "../util/macos_window.h"
#endif

void SettingsView::closeSettingsWindow(EditorApi &api)
{
	s.showSettingsWindow = false;
	s.saveSettings();
	api.setBlockInput(false);
}

bool SettingsView::apply(bool force, EditorApi &api)
{
	if (!s.needsApply && !force)
		return false;
	s.needsApply = false;

	// Never call .value() on null / non-object (throws type_error.306).
	if (!s.settings.is_object())
	{
		std::cerr << "[Settings] apply: s.settings JSON is not an object; "
					 "loading bundled defaults"
				  << std::endl;
		std::string path;
		if (!s.loadBundledProfile(s.settings, path))
			return false;
		if (s.settingsPath.empty())
			s.settingsPath = path;
	}

	font.setFont(s.settings.value("font", std::string("SourceCodePro-Regular")),
				   s.settings.value("fontSize", 20.0f));
	font.load();

	ImGuiStyle &style = ImGui::GetStyle();
	ApplySettings(style);

	// Embedded hosts keep the host ImGui theme for window/child backgrounds.
	// Standalone: match window, child, and editor tab bar to theme background.
	if (!s.isEmbedded && s.settings.contains("backgroundColor") &&
		s.settings["backgroundColor"].is_array() &&
		s.settings["backgroundColor"].size() >= 3)
	{
		const auto &bg = s.settings["backgroundColor"];
		const float a =
			s.settings["backgroundColor"].size() >= 4 ? bg[3].get<float>() : 1.0f;
		const ImVec4 bgCol(bg[0].get<float>(), bg[1].get<float>(), bg[2].get<float>(), a);
		const ImVec4 bgOpaque(
			bg[0].get<float>(), bg[1].get<float>(), bg[2].get<float>(), 1.0f);
		style.Colors[ImGuiCol_ChildBg] = bgOpaque;
		style.Colors[ImGuiCol_WindowBg] = bgCol;
		// Dock title-bar tabs: no fill by default; slight highlight only on hover.
		const ImVec4 tabNone(0.0f, 0.0f, 0.0f, 0.0f);
		style.Colors[ImGuiCol_Tab] = tabNone;
		style.Colors[ImGuiCol_TabSelected] = tabNone;
		style.Colors[ImGuiCol_TabDimmed] = tabNone;
		style.Colors[ImGuiCol_TabDimmedSelected] = tabNone;
		// Soft lift from text color so hover reads on light and dark themes.
		const ImVec4 &text = style.Colors[ImGuiCol_Text];
		style.Colors[ImGuiCol_TabHovered] = ImVec4(text.x, text.y, text.z, 0.12f);
		// Window/tab close (X) draws ButtonHovered / ButtonActive as its fill.
		style.Colors[ImGuiCol_Button] = ImVec4(text.x, text.y, text.z, 0.12f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(text.x, text.y, text.z, 0.18f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(text.x, text.y, text.z, 0.30f);
		// Hide selected-tab overline so active doesn't look different.
		style.Colors[ImGuiCol_TabSelectedOverline] = tabNone;
		style.Colors[ImGuiCol_TabDimmedSelectedOverline] = tabNone;
		// Window / dock title bars (active, inactive, collapsed) match theme bg.
		style.Colors[ImGuiCol_TitleBg] = bgOpaque;
		style.Colors[ImGuiCol_TitleBgActive] = bgOpaque;
		style.Colors[ImGuiCol_TitleBgCollapsed] = bgOpaque;
		style.Colors[ImGuiCol_MenuBarBg] = bgOpaque;
		style.Colors[ImGuiCol_DockingEmptyBg] = bgOpaque;
		// Hide the dock title-bar / tab-bar window-menu (tab list) button.
		// See https://github.com/ocornut/imgui/issues/4880
		style.WindowMenuButtonPosition = ImGuiDir_None;
	}

	s.sidebarVisible = s.settings.value("sidebar_visible", true);
	s.terminalVisible = s.settings.value("terminal_visible", true);

	api.forceColorUpdate();

#ifdef __APPLE__
	updateMacOSWindowProperties(s.settings.value("mac_background_opacity", 0.5f),
								s.settings.value("mac_blur_enabled", true));
#endif
	return true;
}

void SettingsView::ApplySettings(ImGuiStyle &style)
{
	if (!s.settings.is_object())
		return;

	// Standalone only: embedded mode leaves WindowBg/ChildBg to the host style.
	if (!s.isEmbedded && s.settings.contains("backgroundColor") &&
		s.settings["backgroundColor"].is_array() &&
		s.settings["backgroundColor"].size() >= 4)
	{
		const auto &bg = s.settings["backgroundColor"];
		style.Colors[ImGuiCol_WindowBg] = ImVec4(bg[0].get<float>(),
												 bg[1].get<float>(),
												 bg[2].get<float>(),
												 bg[3].get<float>());
	}

	const std::string theme = s.settings.value("theme", std::string("default"));
	if (!s.settings.contains("themes") || !s.settings["themes"].is_object() ||
		!s.settings["themes"].contains(theme) ||
		!s.settings["themes"][theme].contains("text"))
		return;

	const auto &textColor = s.settings["themes"][theme]["text"];
	if (!textColor.is_array() || textColor.size() < 4)
		return;
	ImVec4 textCol(textColor[0].get<float>(),
				   textColor[1].get<float>(),
				   textColor[2].get<float>(),
				   textColor[3].get<float>());
	style.Colors[ImGuiCol_Text] = textCol;
	style.Colors[ImGuiCol_TextDisabled] =
		ImVec4(textCol.x * 0.6f, textCol.y * 0.6f, textCol.z * 0.6f, textCol.w);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.0f, 0.1f, 0.7f, 0.3f);

	if (!s.isEmbedded)
	{
		const float dpi = std::max(1.0f, style.FontScaleDpi);
		style.ScrollbarSize = 30.0f * dpi;
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0, 0, 0, 0);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0, 0, 0, 0);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0, 0, 0, 0);
	} else
	{
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
	}
	style.ScaleAllSizes(1.0f);
}

std::string SettingsView::displayFontName(const std::string &fontFile)
{
	if (fontFile == "System Default" || fontFile.find('.') == std::string::npos)
		return fontFile;
	std::string name = fontFile.substr(0, fontFile.find_last_of('.'));
	std::replace(name.begin(), name.end(), '-', ' ');
	return name;
}

void SettingsView::renderSettingsWindow(EditorApi &api,
										FileExplorer &files,
										LspImGuiView &lsp)
{
	if (!s.showSettingsWindow)
		return;

	if (s.isEmbedded)
	{
		ImGui::SetNextWindowPos(ImVec2(s.embeddedWindowPos.x, s.embeddedWindowPos.y),
								  ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(s.embeddedWindowSize.x, s.embeddedWindowSize.y),
								   ImGuiCond_FirstUseEver);

		bool windowOpen = true;
		if (ImGui::Begin("Settings", &windowOpen, ImGuiWindowFlags_NoCollapse))
		{
			s.embeddedWindowPos = {ImGui::GetWindowPos().x, ImGui::GetWindowPos().y};
			s.embeddedWindowSize = {ImGui::GetWindowSize().x, ImGui::GetWindowSize().y};
			if (!windowOpen)
				s.showSettingsWindow = false;
			renderSettingsContent(api, files, lsp);
			ImGui::End();
		}
		return;
	}

	const float fontSize = s.settings.value("fontSize", 20.0f);
	const ImVec2 viewport = ImGui::GetMainViewport()->Size;
	const float width =
		viewport.x * ((viewport.x < 1100.0f || fontSize > 40.0f) ? 0.90f : 0.75f);
	const float height =
		viewport.y * ((viewport.x < 1100.0f || fontSize > 40.0f) ? 0.80f : 0.85f);

	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
	ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
	ImGui::SetNextWindowPos(
		ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f),
		ImGuiCond_Always,
		ImVec2(0.5f, 0.5f));

	applyImGuiStyles();
	ImGui::Begin("Settings",
				 nullptr,
				 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
					 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
					 ImGuiWindowFlags_Modal);
	ImGui::PushFont(font.getMainFont());
	renderSettingsContent(api, files, lsp);
	ImGui::PopFont();
	ImGui::End();
	ImGui::PopStyleColor(8);
	ImGui::PopStyleVar(6);
}

void SettingsView::renderSettingsContent(EditorApi &api,
										 FileExplorer &files,
										 LspImGuiView &lsp)
{
	if (!s.isEmbedded)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		renderWindowHeader(api, files);
		ImGui::PopStyleVar();
	}

	const bool pushBg = !s.isEmbedded && s.settings.contains("backgroundColor") &&
						s.settings["backgroundColor"].is_array() &&
						s.settings["backgroundColor"].size() >= 3;
	if (pushBg)
	{
		const auto &bg = s.settings["backgroundColor"];
		const float m = 0.8f;
		ImGui::PushStyleColor(ImGuiCol_ChildBg,
							  ImVec4(bg[0].get<float>() * m,
									 bg[1].get<float>() * m,
									 bg[2].get<float>() * m,
									 1.0f));
	}
	ImGui::PushStyleVar(
		ImGuiStyleVar_WindowPadding,
		ImVec2(ImGui::GetFontSize() * 0.75f, ImGui::GetFontSize() * 0.25f));
	ImGui::BeginChild("SettingsContent",
					  ImVec2(0, ImGui::GetContentRegionAvail().y),
					  false,
					  ImGuiWindowFlags_AlwaysVerticalScrollbar);

	renderProfileSelector();
	renderMainSettings();
	if (!s.isEmbedded)
		renderMacSettings();
	renderSyntaxColors();
	renderToggleSettings();
#if NED_ENABLE_SHADERS
	if (!s.isEmbedded)
		renderShaderSettings();
#endif
	renderKeybindsSettings(files, lsp);

	ImGui::EndChild();
	if (pushBg)
		ImGui::PopStyleColor();
	ImGui::PopStyleVar();
	handleWindowInput(api);
}

void SettingsView::applyImGuiStyles()
{
	const float fs = ImGui::GetFontSize();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, fs * 0.5f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, fs * 0.7f);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, fs * 0.5f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(fs * 0.75f, fs * 0.75f));

	// Embedded: keep host ImGui palette for window/frame backgrounds.
	if (s.isEmbedded)
	{
		// Still push the same color stack depth so PopStyleColor(8) stays balanced.
		const ImGuiStyle &s = ImGui::GetStyle();
		ImGui::PushStyleColor(ImGuiCol_WindowBg, s.Colors[ImGuiCol_WindowBg]);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, s.Colors[ImGuiCol_FrameBg]);
		ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, s.Colors[ImGuiCol_ScrollbarBg]);
		ImGui::PushStyleColor(ImGuiCol_PopupBg, s.Colors[ImGuiCol_PopupBg]);
		ImGui::PushStyleColor(ImGuiCol_Border, s.Colors[ImGuiCol_Border]);
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, s.Colors[ImGuiCol_ScrollbarGrab]);
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered,
							  s.Colors[ImGuiCol_ScrollbarGrabHovered]);
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,
							  s.Colors[ImGuiCol_ScrollbarGrabActive]);
		return;
	}

	const float windowMul = 0.8f;
	const float frameMul = 0.5f;
	const float border = 0.3f;
	const auto &bg = s.settings["backgroundColor"];
	const float r = bg[0].get<float>();
	const float g = bg[1].get<float>();
	const float b = bg[2].get<float>();

	ImGui::PushStyleColor(ImGuiCol_WindowBg,
						  ImVec4(r * windowMul, g * windowMul, b * windowMul, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg,
						  ImVec4(r * frameMul, g * frameMul, b * frameMul, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,
						  ImVec4(r * frameMul, g * frameMul, b * frameMul, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_PopupBg,
						  ImVec4(r * frameMul, g * frameMul, b * frameMul, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(border, border, border, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(border, border, border, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,
						  ImVec4(frameMul, frameMul, frameMul, 1.0f));
}

void SettingsView::renderWindowHeader(EditorApi &api, FileExplorer &files)
{
	static bool wasFocused = false;
	const bool isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	const bool windowHovered =
		ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
	// Layout changes (terminal/sidebar) can steal ImGui focus for a frame.
	// Don't dismiss if the pointer is still over Settings — click-outside
	// is handled in handleWindowInput.
	if (wasFocused && !isFocused && s.showSettingsWindow && !windowHovered)
		closeSettingsWindow(api);
	wasFocused = isFocused;

	ImGui::BeginGroup();
	ImGui::TextUnformatted("Settings");
	const float closeSize = ImGui::GetFontSize();
	const float buttonX = ImGui::GetContentRegionAvail().x - closeSize -
						  ImGui::GetStyle().FramePadding.x * 2;
	ImGui::SameLine(buttonX > 0 ? buttonX : ImGui::GetCursorPosX() + 100);

	const ImVec2 cursor = ImGui::GetCursorPos();
	if (ImGui::InvisibleButton("##close-settings", ImVec2(closeSize, closeSize)))
		closeSettingsWindow(api);
	const bool hovered = ImGui::IsItemHovered();
	ImGui::SetCursorPos(cursor);
	ImGui::Image(ImTextureRef(files.icons.get("close")),
				 ImVec2(closeSize, closeSize),
				 ImVec2(0, 0),
				 ImVec2(1, 1),
				 hovered ? ImVec4(1, 1, 1, 0.6f) : ImVec4(1, 1, 1, 1),
				 ImVec4(0, 0, 0, 0));
	ImGui::EndGroup();
	ImGui::Separator();
}

void SettingsView::renderProfileSelector()
{
	ImGui::Spacing();

	std::vector<std::string> profiles = s.listProfiles();
	std::string current = s.settingsPath.empty()
							  ? "ned.json"
							  : fs::path(s.settingsPath).filename().string();

	if (std::find(profiles.begin(), profiles.end(), current) == profiles.end() &&
		!current.empty())
		profiles.insert(profiles.begin(), current);

	if (ImGui::BeginCombo("##ActiveSettingsFileCombo", current.c_str()))
	{
		for (const std::string &name : profiles)
		{
			const bool selected = (current == name);
			if (ImGui::Selectable(name.c_str(), selected) && !selected)
				s.switchToProfile(name);
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	ImGui::TextUnformatted("Profile");
	ImGui::Spacing();
}

void SettingsView::renderMainSettings()
{
	const std::string currentFont =
		s.settings.value("font", std::string("SourceCodePro-Regular"));
	if (ImGui::BeginCombo("Font", displayFontName(currentFont).c_str()))
	{
		for (const auto &fontFile : font.availableFonts())
		{
			const bool selected = (fontFile == currentFont);
			if (ImGui::Selectable(displayFontName(fontFile).c_str(), selected))
			{
				s.settings["font"] = fontFile;
				s.needsApply = true;
				s.saveSettings();
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	ImGui::Spacing();

	float fontSize = s.settings.value("fontSize", 20.0f);
	if (ImGui::SliderFloat("Font Size", &fontSize, 4.0f, 64.0f, "%.0f"))
	{
		s.settings["fontSize"] = fontSize;
		s.needsApply = true;
	}
	if (ImGui::IsItemDeactivatedAfterEdit())
		s.saveSettings();

	ImGui::Spacing();

	ImVec4 bgColor(0.058f, 0.194f, 0.158f, 1.0f);
	if (s.settings.contains("backgroundColor") &&
		s.settings["backgroundColor"].is_array() &&
		s.settings["backgroundColor"].size() == 4)
	{
		const auto &bg = s.settings["backgroundColor"];
		bgColor = ImVec4(bg[0].get<float>(),
						 bg[1].get<float>(),
						 bg[2].get<float>(),
						 bg[3].get<float>());
	}
	if (ImGui::ColorEdit4("Background Color", (float *)&bgColor))
	{
		s.settings["backgroundColor"] = {bgColor.x, bgColor.y, bgColor.z, bgColor.w};
		s.needsApply = true;
		s.saveSettings();
	}
}

void SettingsView::renderMacSettings()
{
#ifdef __APPLE__
	ImGui::Spacing();
	ImGui::TextUnformatted("macOS Settings");
	ImGui::Separator();
	ImGui::Spacing();

	float opacity = s.settings.value("mac_background_opacity", 0.5f);
	if (ImGui::SliderFloat("Background Opacity", &opacity, 0.0f, 1.0f, "%.2f"))
	{
		s.settings["mac_background_opacity"] = opacity;
		s.needsApply = true;
		s.saveSettings();
	}

	bool blur = s.settings.value("mac_blur_enabled", true);
	if (ImGui::Checkbox("Enable Background Blur", &blur))
	{
		s.settings["mac_blur_enabled"] = blur;
		s.needsApply = true;
		s.saveSettings();
	}
#endif
}

void SettingsView::renderSyntaxColors()
{
	const std::string theme = s.settings.value("theme", std::string("default"));
	if (!s.settings.contains("themes") || !s.settings["themes"].is_object() ||
		!s.settings["themes"].contains(theme))
	{
		ImGui::Text("Theme '%s' not found.", theme.c_str());
		return;
	}

	auto &colors = s.settings["themes"][theme];

	// Ensure key exists so older 8-slot themes can gain extras from the picker.
	auto ensureColor = [&](const char *key, const char *fallbackKey) {
		if (colors.contains(key) && colors[key].is_array() && colors[key].size() == 4)
			return;
		if (colors.contains(fallbackKey) && colors[fallbackKey].is_array() &&
			colors[fallbackKey].size() == 4)
			colors[key] = colors[fallbackKey];
		else
			colors[key] = {0.75f, 0.75f, 0.75f, 1.0f};
	};

	// Core
	ensureColor("text", "text");
	ensureColor("keyword", "text");
	ensureColor("string", "text");
	ensureColor("number", "text");
	ensureColor("comment", "text");
	ensureColor("function", "text");
	ensureColor("type", "text");
	ensureColor("variable", "text");
	// Extended (plan: ~15 slots)
	ensureColor("parameter", "variable");
	ensureColor("property", "variable");
	ensureColor("constant", "number");
	ensureColor("operator", "text");
	ensureColor("punctuation", "text");
	ensureColor("special", "keyword");

	auto editColor = [&](const char *label, const char *key) {
		if (!colors.contains(key) || !colors[key].is_array() || colors[key].size() != 4)
			return;
		auto &arr = colors[key];
		ImVec4 color(arr[0].get<float>(),
					 arr[1].get<float>(),
					 arr[2].get<float>(),
					 arr[3].get<float>());

		ImGui::TextUnformatted(label);
		ImGui::SameLine(200);
		if (ImGui::ColorEdit4(("##" + std::string(key)).c_str(), (float *)&color))
		{
			colors[key] = {color.x, color.y, color.z, color.w};
			s.needsApply = true;
		}
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			colors[key] = {color.x, color.y, color.z, color.w};
			s.needsApply = true;
			s.saveSettings();
		}
	};

	ImGui::Spacing();
	if (ImGui::CollapsingHeader("Syntax Colors"))
	{
		ImGui::Spacing();
		editColor("Text", "text");
		editColor("Keywords", "keyword");
		editColor("Strings", "string");
		editColor("Numbers", "number");
		editColor("Comments", "comment");
		editColor("Functions", "function");
		editColor("Types", "type");
		editColor("Identifier", "variable");
		editColor("Parameter", "parameter");
		editColor("Property / field", "property");
		editColor("Constant", "constant");
		editColor("Operator", "operator");
		editColor("Punctuation", "punctuation");
		editColor("Special / builtin", "special");
	}
}

void SettingsView::renderToggleSettings()
{
	ImGui::Spacing();
	ImGui::TextUnformatted("Toggle Settings");
	ImGui::Separator();
	ImGui::Spacing();

	bool sidebar = s.settings.value("sidebar_visible", true);
	if (ImGui::Checkbox("File Explorer", &sidebar))
		s.toggleSidebar();
	ImGui::SameLine();
	ImGui::TextDisabled("(Show/hide file explorer sidebar)");
	ImGui::Spacing();

	bool term = s.settings.value("terminal_visible", true);
	if (ImGui::Checkbox("Terminal", &term))
		s.toggleTerminal();
	ImGui::SameLine();
	ImGui::TextDisabled("(Show/hide bottom terminal panel)");
	ImGui::Spacing();

	bool rainbow = s.settings.value("rainbow", true);
	if (ImGui::Checkbox("Rainbow Mode", &rainbow))
	{
		s.settings["rainbow"] = rainbow;
		s.saveSettings();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(Rainbow cursor & line numbers)");

	bool minimap = s.settings.value("minimap", true);
	if (ImGui::Checkbox("Minimap", &minimap))
	{
		s.settings["minimap"] = minimap;
		s.saveSettings();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(Code overview strip on the right)");

	bool wordWrap = s.settings.value("word_wrap", false);
	if (ImGui::Checkbox("Word Wrap", &wordWrap))
	{
		s.settings["word_wrap"] = wordWrap;
		s.saveSettings();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(Wrap long lines to the window width)");

	bool treesitter = s.settings.value("treesitter", true);
	if (ImGui::Checkbox("TreeSitter Mode", &treesitter))
	{
		s.settings["treesitter"] = treesitter;
		s.needsApply = true;
		s.saveSettings();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(Syntax Highlighting)");

#if NED_ENABLE_GIT
	bool gitLines = s.settings.value("git_changed_lines", true);
	if (ImGui::Checkbox("Git Changed Lines", &gitLines))
	{
		s.settings["git_changed_lines"] = gitLines;
		s.saveSettings();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(Highlight changed lines in git)");
#endif
}

void SettingsView::renderShaderSettings()
{
	ImGui::Spacing();
	ImGui::TextUnformatted("GL Shaders");
	ImGui::Separator();
	ImGui::Spacing();

	bool enabled = s.settings.value("shader_toggle", true);
	if (ImGui::Checkbox("Enable Shader Effects", &enabled))
	{
		s.settings["shader_toggle"] = enabled;
		s.saveSettings();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(CRT & visual effects)");

	renderShaderSlider("Scanline", "scanline_intensity", 0.00f, 1.00f, "%.02f", 0.20f);
	renderShaderSlider("Vignette", "vignet_intensity", 0.00f, 1.00f, "%.02f", 0.25f);
	renderShaderSlider("Bloom", "bloom_intensity", 0.00f, 1.00f, "%.02f", 0.75f);
	renderShaderSlider("Static", "static_intensity", 0.00f, 0.5f, "%.03f", 0.208f);
	renderShaderSlider("RGB Shift", "colorshift_intensity", 0.0f, 10.0f, "%.02f", 0.90f);
	renderShaderSlider(
		"Curvature(bugged)", "curvature_intensity", 0.0f, 0.5f, "%.02f", 0.0f);
	renderShaderSlider("Burn-in", "burnin_intensity", 0.9f, 0.999f, "%.03f", 0.9525f);
	renderShaderSlider("Jitter", "jitter_intensity", 0.0f, 10.0f, "%.02f", 2.81f);
	renderShaderSlider(
		"Pixel lines", "pixelation_intensity", -1.00f, 1.00f, "%.03f", -0.11f);
	renderShaderSlider("FPS Target", "fps_target", 20.0f, 1000.0f, "%.0f", 120.0f);
}

void SettingsView::renderShaderSlider(const char *label,
									  const char *key,
									  float min_val,
									  float max_val,
									  const char *format,
									  float default_val)
{
	float value = s.settings.value(key, default_val);
	if (ImGui::SliderFloat(
			label, &value, min_val, max_val, format, ImGuiSliderFlags_AlwaysClamp))
		s.settings[key] = value;
	if (ImGui::IsItemDeactivatedAfterEdit())
		s.saveSettings();
	ImGui::Spacing();
}

void SettingsView::renderKeybindsSettings(FileExplorer &files, LspImGuiView &lsp)
{
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	const fs::path configDir = s.getUserConfigDir();
	const std::string keybindsPath = (configDir / "keybinds.json").string();
	const std::string defaultKeybindsPath =
		(configDir / "default-s.keybinds.json").string();

	if (ImGui::Button("Open Keybinds File"))
	{
		if (fs::exists(keybindsPath))
		{
			files.loadFileContent(keybindsPath);
			s.showSettingsWindow = false;
		} else
		{
			std::cerr << "[Settings] Keybinds file not found: " << keybindsPath
					  << std::endl;
		}
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(Edit keyboard shortcuts)");

	if (fs::exists(defaultKeybindsPath) && !fs::exists(keybindsPath))
	{
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Using default s.keybinds");
		if (ImGui::Button("Restore Default Keybinds"))
		{
			try
			{
				fs::copy_file(defaultKeybindsPath,
							  keybindsPath,
							  fs::copy_options::overwrite_existing);
				s.keybinds.loadKeybinds();
			} catch (const fs::filesystem_error &e)
			{
				std::cerr << "[Settings] Error restoring s.keybinds: " << e.what()
						  << std::endl;
			}
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(Reset to default configuration)");
	}

#if NED_ENABLE_LSP
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	if (ImGui::Button("LSP Dashboard"))
	{
		lsp.dashboard.setShow(true);
		s.showSettingsWindow = false;
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(View LSP server status)");
#else
	(void)lsp;
#endif
}

void SettingsView::handleWindowInput(EditorApi &api)
{
	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		closeSettingsWindow(api);
		return;
	}

	if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsAnyItemHovered() ||
		ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow |
							   ImGuiHoveredFlags_AllowWhenBlockedByPopup) ||
		ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId))
		return;

	const ImVec2 mouse = ImGui::GetMousePos();
	const ImVec2 pos = ImGui::GetWindowPos();
	const ImVec2 size = ImGui::GetWindowSize();
	if (mouse.x < pos.x || mouse.x > pos.x + size.x || mouse.y < pos.y ||
		mouse.y > pos.y + size.y)
		closeSettingsWindow(api);
}

void SettingsView::renderNotification()
{
	if (s.notificationTimer <= 0.0f)
		return;

	ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float padding = 20.0f;
	const float textPad = 15.0f;
	const float maxWidth = viewport->Size.x * 0.8f;
	const ImVec2 textSize = ImGui::CalcTextSize(
		s.notificationText.c_str(), nullptr, false, maxWidth - textPad * 2);
	const float width = std::clamp(textSize.x + textPad * 2, 200.0f, maxWidth);
	const float height =
		std::clamp(textSize.y + textPad * 2, 50.0f, viewport->Size.y * 0.4f);
	const ImVec2 origin(viewport->Pos.x + padding,
						viewport->Pos.y + viewport->Size.y - height - padding);

	const auto &bg = s.settings["backgroundColor"];
	ImDrawList *draw = ImGui::GetForegroundDrawList();
	draw->AddRectFilled(origin,
						ImVec2(origin.x + width, origin.y + height),
						IM_COL32(static_cast<int>(bg[0].get<float>() * 255),
								 static_cast<int>(bg[1].get<float>() * 255),
								 static_cast<int>(bg[2].get<float>() * 255),
								 230),
						8.0f);
	draw->AddRect(origin,
				  ImVec2(origin.x + width, origin.y + height),
				  IM_COL32(255, 255, 255, 255),
				  8.0f,
				  0,
				  1.0f);
	draw->AddText(ImVec2(origin.x + textPad, origin.y + textPad),
				  IM_COL32(255, 255, 255, 255),
				  s.notificationText.c_str());

	s.notificationTimer -= ImGui::GetIO().DeltaTime;
}
