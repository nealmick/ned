#include "file_sidebar_view.h"
#include "../../../files/files.h"
#include "../../../util/imgui_icons.h"
#include "../../../util/settings.h"
#include "imgui.h"

#include <algorithm>

#include "file_tree_view.h"

void renderFileSidebar(FileExplorer &fx, float explorerWidth)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);

	const float treeW = explorerWidth <= 0.0f ? 0.0f : explorerWidth;
	// Standalone macOS / Windows: explorer/terminal/fx.settings live in the
	// window title bar, so the explorer doesn't grow a second toolbar.
	const bool nativeTitlebar =
#if defined(__APPLE__) || defined(_WIN32)
		fx.settings && !fx.settings->isEmbedded;
#else
		false;
#endif
	const float icon = ImGui::GetFontSize() * 1.05f;
	const float pad = ImGui::GetFontSize() * 0.5f;
	const float barH = nativeTitlebar ? 0.0f : icon + ImGui::GetFontSize() * 0.8f;
	// NoScrollbar: the app style sizes the scrollbar lane at 30px but paints
	// it fully transparent, so an overflowing tree just lost that strip on
	// the right (rows stopped short of the splitter). Wheel scrolling works
	// without the bar — same treatment as ##ned_terminal_host.
	ImGui::BeginChild("File Tree",
					  ImVec2(treeW, barH > 0.0f ? -barH : 0.0f),
					  ImGuiChildFlags_None,
					  ImGuiWindowFlags_NoScrollbar);
	if (!fx.projectRoot.empty())
		renderFileTree(fx.fileTree, fx.fileTree.rootNode);
	ImGui::EndChild();

	if (barH > 0.0f)
	{
		const ImVec4 bg = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
		const ImVec4 tx = ImGui::GetStyleColorVec4(ImGuiCol_Text);
		ImGui::PushStyleColor(ImGuiCol_ChildBg,
							  ImVec4(bg.x + (tx.x - bg.x) * 0.1f,
									 bg.y + (tx.y - bg.y) * 0.1f,
									 bg.z + (tx.z - bg.z) * 0.1f,
									 bg.w));
		ImGui::BeginChild(
			"##explorer_activity", ImVec2(0.0f, barH), 0, ImGuiWindowFlags_NoScrollbar);
		ImGui::SetCursorPos(ImVec2(pad, (barH - icon) * 0.5f));
		const ImVec2 sz(icon, icon);
		auto btn = [&](const char *id, const char *off, const char *on, const char *tip) {
			const ImVec2 p = ImGui::GetCursorPos();
			const bool hit = ImGui::InvisibleButton(id, sz);
			const bool hov = ImGui::IsItemHovered();
			if (hov)
				ImGui::SetTooltip("%s", tip);
			ImGui::SetCursorPos(p);
			ImGui::Image(fx.icons.get(hov ? on : off), sz);
			return hit;
		};
		if (btn("##settings", "gear", "gear-hover", "Settings") && fx.api && fx.settings)
			fx.settings->toggleSettingsWindow(*fx.api);
		ImGui::SameLine(0.0f, pad);
		if (btn("##terminal", "terminal", "terminal-hover", "Terminal") && fx.settings)
			fx.settings->toggleTerminal();
		ImGui::EndChild();
		ImGui::PopStyleColor();
	}
	ImGui::PopStyleVar(4);
}
