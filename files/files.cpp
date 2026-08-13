/*
	File: files.cpp
	Description: File explorer — disk + project UI; document open goes through EditorApi.
*/

#include "files.h"

#include "../editor/editor_api.h"
#include "../util/settings.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <nfd.h>
#include <vector>

FileExplorer::FileExplorer(EditorApi &api,
						   Settings &settings,
						   std::string &projectRoot,
						   Icons &icons)
	: projectRoot(projectRoot), icons(icons)
{
	this->api = &api;
	this->settings = &settings;

	fileTree.fileExplorer = this;
	fileTree.settings = this->settings;

	fileFinder.fileExplorer = this;
	fileFinder.settings = this->settings;
	fileFinder.startBackgroundThread();
}

void FileExplorer::openFolderDialog()
{
	nfdchar_t *outPath = nullptr;
	const nfdresult_t result = NFD_PickFolder(nullptr, &outPath);
	showFileDialog = false;

	if (result != NFD_OKAY)
	{
		if (result != NFD_CANCEL)
			std::cerr << "Folder dialog error: " << NFD_GetError() << std::endl;
		return;
	}

	projectRoot = outPath;
	free(outPath);
	showWelcomeScreen = false;
	if (api)
		api->onProjectOpened(projectRoot);
}

bool FileExplorer::handleFileDialog()
{
	if (!showFileDialog)
		return false;

	const std::string previous = projectRoot;
	openFolderDialog();
	if (projectRoot.empty() || projectRoot == previous)
		return false;

	auto &root = fileTree.rootNode;
	root.name = fs::path(projectRoot).filename().string();
	root.fullPath = projectRoot;
	root.isDirectory = true;
	root.isOpen = true;
	root.children.clear();
	fileTree.buildFileTree(projectRoot, root);
	showWelcomeScreen = false;

	events.emitDidOpenProject({projectRoot});
	return true;
}

bool FileExplorer::readFileRaw(const std::string &path, std::string &raw)
{
	try
	{
		if (!fs::exists(path) || !fs::is_regular_file(path))
			return false;

		std::error_code ec;
		const uintmax_t fileSize = fs::file_size(path, ec);
		if (ec)
			return false;

		const bool truncated = fileSize > MAX_FILE_SIZE;
		std::ifstream file(path, std::ios::binary);
		if (!file)
			return false;

		const size_t readSize = truncated ? MAX_FILE_SIZE : static_cast<size_t>(fileSize);
		std::vector<char> buffer(readSize);
		file.read(buffer.data(), static_cast<std::streamsize>(readSize));
		if (file.bad())
			return false;

		raw.assign(buffer.data(), static_cast<size_t>(file.gcount()));

		const size_t checkSize = std::min(raw.size(), size_t{1024});
		int junk = 0;
		for (size_t i = 0; i < checkSize; ++i)
		{
			const unsigned char c = static_cast<unsigned char>(raw[i]);
			if (c == 0 || (c < 32 && c != '\n' && c != '\r' && c != '\t'))
				++junk;
		}
		if (checkSize > 0 && junk > static_cast<int>(checkSize / 10))
		{
			raw = "Error: File appears to be binary and cannot be displayed.";
			return false;
		}

		if (truncated)
		{
			const std::string notice = "\n\n[File truncated - No Edits - showing first " +
									   std::to_string(MAX_FILE_SIZE / (1024 * 1024)) +
									   "MB of " +
									   std::to_string(fileSize / (1024 * 1024)) + "MB]\n";
			raw = notice + raw;
		}

		return true;
	} catch (const std::exception &)
	{
		return false;
	}
}

void FileExplorer::setEditorsBlockInput(bool blocked)
{
	if (blockInputOverride)
		blockInputOverride(blocked);
	else if (api)
		api->setBlockInput(blocked);
}

void FileExplorer::loadFileContent(const std::string &path,
								   std::function<void()> afterLoadCallback)
{
	// Embed multi-tab (or other hosts) can own open/focus.
	if (openOverride)
	{
		openOverride(path, std::move(afterLoadCallback));
		return;
	}

	if (!api)
		return;

	// Single-buffer: remember prior path so LSP can didClose before the new didOpen.
	const std::string previousPath = api->hasPath() ? api->path() : std::string();

	api->prepareLoad();

	std::string raw;
	if (!readFileRaw(path, raw))
	{
		// Drop the previous document from LSP even when the new open fails.
		if (!previousPath.empty())
			events.emitDidCloseDocument({previousPath});
		api->failOpen(raw.empty() ? "Error: Unable to open file." : raw);
		return;
	}

	api->openDocument(path, raw);

	// Same path reload → keep open (didOpen becomes didChange via LSP tracking).
	if (!previousPath.empty() && previousPath != path)
		events.emitDidCloseDocument({previousPath});

	events.emitDidOpenDocument({path});

	if (afterLoadCallback)
		afterLoadCallback();
}

void FileExplorer::renderFileExplorer(float explorerWidth)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);

	const float treeW = explorerWidth <= 0.0f ? 0.0f : explorerWidth;
	// Standalone macOS uses the native title-bar controls instead.
	const bool nativeTitlebar =
#ifdef __APPLE__
		settings && !settings->isEmbedded;
#else
		false;
#endif
	const float icon = ImGui::GetFontSize() * 1.05f;
	const float pad = 10.0f;
	const float barH = nativeTitlebar ? 0.0f : icon + 16.0f;
	ImGui::BeginChild("File Tree", ImVec2(treeW, barH > 0.0f ? -barH : 0.0f));
	if (!projectRoot.empty())
		fileTree.displayFileTree(fileTree.rootNode);
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
			ImGui::Image(icons.get(hov ? on : off), sz);
			return hit;
		};
		if (btn("##settings", "gear", "gear-hover", "Settings") && api && settings)
			settings->toggleSettingsWindow(*api);
		ImGui::SameLine(0.0f, pad);
		if (btn("##terminal", "terminal", "terminal-hover", "Terminal") && settings)
			settings->toggleTerminal();
		ImGui::EndChild();
		ImGui::PopStyleColor();
	}
	ImGui::PopStyleVar(4);
}

void FileExplorer::renderFileFinder() { fileFinder.renderWindow(); }
