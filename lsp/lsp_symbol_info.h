#pragma once

#include "imgui.h"
#include <future>
#include <string>

class EditorApi;
class FileExplorer;
class LSPClient;
class Settings;

class LSPSymbolInfo
{
  public:
	LSPSymbolInfo(LSPClient &client,
				  EditorApi &api,
				  FileExplorer &fileExplorer,
				  Settings &settings);
	~LSPSymbolInfo();

	// Check keybind and trigger symbol info if conditions are met
	void get();

	// Request hover information from LSP
	void
	request(int line, int character, std::function<void(const std::string &)> callback);

	// Render the symbol info UI
	void render();

	void setApi(EditorApi &editorApi) { api = &editorApi; }

  private:
	// State
	bool show;
	std::string symbolInfo;
	LSPClient *client = nullptr;
	EditorApi *api = nullptr;
	FileExplorer *fileExplorer = nullptr;
	Settings *settings = nullptr;

	// Async request handling
	bool pending;
};
