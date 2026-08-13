/*
	File: views/title_bar_view.cpp
	Description: Title bar rendering (path, git changes).
*/
#include "title_bar_view.h"
#include "../../util/icons.h"
#include "../../util/settings.h"
#include "../services/git/git_service.h"
#include "imgui.h"
#include <algorithm>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

std::string TitleBarView::normalizePathForDisplay(const std::string &path)
{
#ifdef PLATFORM_WINDOWS
	std::string result = path;
	std::replace(result.begin(), result.end(), '/', '\\');

	// Collapse consecutive backslashes, but keep the leading \\ of a UNC path.
	std::string cleaned;
	cleaned.reserve(result.size());
	bool lastWasBackslash = false;
	const bool isUNC = result.size() >= 2 && result[0] == '\\' && result[1] == '\\';

	for (size_t i = 0; i < result.size(); ++i)
	{
		char c = result[i];
		if (c == '\\')
		{
			if (!lastWasBackslash || (i == 1 && isUNC))
			{
				cleaned += c;
				lastWasBackslash = true;
			}
		} else
		{
			cleaned += c;
			lastWasBackslash = false;
		}
	}
	return cleaned;
#else
	return path;
#endif
}

std::string TitleBarView::truncateFilePath(const std::string &path, float maxWidth)
{
	if (path.empty())
		return {};

	auto fits = [maxWidth](const std::string &s) {
		return ImGui::CalcTextSize(s.c_str()).x <= maxWidth;
	};

	// Split into root (e.g. "/" or "C:\") and remaining components.
	fs::path p(path);
	std::string root;
	std::vector<std::string> parts;

	if (p.has_root_path())
	{
		root = p.root_path().string();
		for (auto it = std::next(p.begin()); it != p.end(); ++it)
		{
			if (!it->empty())
				parts.push_back(it->string());
		}
	} else
	{
		for (const auto &part : p)
		{
			if (!part.empty())
				parts.push_back(part.string());
		}
	}

	if (parts.empty())
		return normalizePathForDisplay(root.empty() ? path : root);

	auto joinFrom = [&parts](size_t start) {
		std::string out;
		for (size_t i = start; i < parts.size(); ++i)
		{
			if (!out.empty())
				out += '/';
			out += parts[i];
		}
		return out;
	};

	// 1) Full path
	std::string full = root + joinFrom(0);
	if (fits(full))
		return normalizePathForDisplay(full);

	// 2) Drop leading directories: .../tail
	for (size_t start = 1; start < parts.size(); ++start)
	{
		std::string candidate = root + ".../" + joinFrom(start);
		if (fits(candidate))
			return normalizePathForDisplay(candidate);
	}

	// 3) Filename only (no ".../" prefix — shorter for relative paths)
	std::string filenameOnly = root + parts.back();
	if (fits(filenameOnly))
		return normalizePathForDisplay(filenameOnly);

	// 4) Character-truncate the filename
	const std::string &name = parts.back();
	for (int len = static_cast<int>(name.size()); len > 0; --len)
	{
		std::string candidate = root + name.substr(0, static_cast<size_t>(len)) + "...";
		if (fits(candidate))
			return normalizePathForDisplay(candidate);
	}

	return normalizePathForDisplay(root + "...");
}

void TitleBarView::render(ImFont *font, const std::string &filePath, bool showGitChanges)
{
	// Light horizontal inset only — parent WindowPadding is 0 so we sit flush
	// under the ImGui dock tab bar (no extra top gap).
	const float fs = ImGui::GetFontSize();
	const float kPadX = fs * 0.4f;
	const float kPadY = fs * 0.1f;
	const ImVec2 cursor = ImGui::GetCursorPos();
	ImGui::SetCursorPos(ImVec2(cursor.x + kPadX, cursor.y + kPadY));

	ImGui::BeginGroup();
	ImGui::PushFont(font);

	const float iconSize = ImGui::GetFontSize() * 1.15f;
	const bool isTerminal = (filePath == "Terminal");
	const bool showGit = showGitChanges && settings->settings["git_changed_lines"] &&
						 !git->currentGitChanges.empty();

	float gitChangesWidth = 0.0f;
	if (showGit)
	{
		gitChangesWidth = ImGui::CalcTextSize(git->currentGitChanges.c_str()).x +
						  ImGui::GetStyle().ItemSpacing.x;
	}

	if (filePath.empty())
	{
		ImGui::Text("Editor - No file selected");
	} else
	{
		ImTextureID fileIcon =
			isTerminal ? icons->get("sh") : icons->getForFile(filePath);
		if (fileIcon)
		{
			float textHeight = ImGui::GetTextLineHeight();
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (textHeight - iconSize) * 0.5f);
			ImGui::Image(fileIcon, ImVec2(iconSize, iconSize));
			ImGui::SameLine();
		}

		float availableWidth = ImGui::GetWindowWidth() - ImGui::GetCursorPosX() -
							   ImGui::GetStyle().ItemSpacing.x - kPadX - gitChangesWidth;
		if (isTerminal)
			availableWidth -= fs * 0.5f;

		std::string label = truncateFilePath(filePath, availableWidth);

		if (isTerminal)
		{
			ImVec2 pos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(pos.x - fs * 0.35f, pos.y + fs * 0.15f));
		}

		ImGui::Text("%s", label.c_str());

		if (showGit)
		{
			ImGui::SameLine();
			ImGui::Text("%s", git->currentGitChanges.c_str());
		}
	}

	ImGui::PopFont();
	ImGui::EndGroup();

	// Terminal uses a custom inset separator; editor uses the standard one.
	if (isTerminal)
	{
		ImDrawList *draw_list = ImGui::GetWindowDrawList();
		ImVec2 p = ImGui::GetCursorScreenPos();
		const float margin = ImGui::GetFontSize() * 0.95f;
		float left = ImGui::GetWindowPos().x + margin;
		float width = ImGui::GetWindowWidth() - margin * 2.0f;
		ImU32 col = ImGui::GetColorU32(ImGuiCol_Separator);
		draw_list->AddLine(ImVec2(left, p.y), ImVec2(left + width, p.y), col);
		ImGui::Dummy(ImVec2(0.0f, 1.0f));
	} else
	{
		ImGui::Separator();
	}
}
