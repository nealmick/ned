/*
	File: files.h
	Description: File explorer — disk, tree, finder. Talks to the editor only via EditorApi.
*/

#pragma once

#include "../util/icons.h"
#include "file_explorer_events.h"
#include "file_finder.h"
#include "file_tree.h"
#include "imgui.h"

#include <filesystem>
#include <functional>
#include <string>

namespace fs = std::filesystem;

class EditorApi;
class Settings;

class FileExplorer
{
  public:
	static constexpr size_t MAX_FILE_SIZE = 1 * 1024 * 1024; // 1 MB

	FileExplorer(EditorApi &api,
				 Settings &settings,
				 std::string &projectRoot,
				 Icons &icons);

	EditorApi *api = nullptr;
	Settings *settings = nullptr;

	std::string &projectRoot;
	Icons &icons;
	FileTree fileTree;
	FileFinder fileFinder;
	FileExplorerEvents events;

	bool showWelcomeScreen = true;
	bool showFileDialog = false;

	void loadFileContent(const std::string &path,
						 std::function<void()> afterLoadCallback = nullptr);

	// When set (e.g. multi-tab embed), replaces prepareLoad/openDocument.
	// Receives absolute path and optional after-open callback.
	std::function<void(const std::string &path, std::function<void()> after)> openOverride;

	// When set, used instead of api->setBlockInput (block every open editor).
	std::function<void(bool blocked)> blockInputOverride;

	// Prefer override when present so multi-tab hosts can mute all editors.
	void setEditorsBlockInput(bool blocked);

	// Read path into raw; false on error (may set raw to an error message).
	bool readFileRaw(const std::string &path, std::string &raw);

	void openFolderDialog();
	bool handleFileDialog();
	void renderFileExplorer(float explorerWidth);
	void renderFileFinder();
};
