#pragma once
#include "../../../lsp/lsp_locations.h"
#include <string>
#include <vector>

class EditorApi;
class EditorSurface;
class FileExplorer;
class Settings;

class LSPUriOptions
{
  public:
	LSPUriOptions(EditorApi &api,
				  EditorSurface &surface,
				  FileExplorer &fileExplorer,
				  Settings &settings);
	~LSPUriOptions();

	void present(const std::string &title,
				 const std::vector<LSPLocation> &options,
				 bool &show);

	void setApi(EditorApi &editorApi, EditorSurface &editorSurface)
	{
		api = &editorApi;
		surface = &editorSurface;
	}

  private:
	void commit();

	std::string currentTitle;
	std::vector<LSPLocation> currentOptions;
	size_t selectedIndex = 0;

	EditorApi *api = nullptr;
	EditorSurface *surface = nullptr; // presentation (pane layout) queries
	FileExplorer *fileExplorer = nullptr;
	Settings *settings = nullptr;
};
