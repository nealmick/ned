#pragma once
#include <map>
#include <string>
#include <vector>

class EditorApi;
class FileExplorer;
class Settings;

class LSPUriOptions
{
  public:
	LSPUriOptions(EditorApi &api, FileExplorer &fileExplorer, Settings &settings);
	~LSPUriOptions();

	void render(const std::string &title,
				const std::vector<std::map<std::string, std::string>> &options,
				bool &show);

	void setApi(EditorApi &editorApi) { api = &editorApi; }

  private:
	void handleSelection();

	std::string currentTitle;
	std::vector<std::map<std::string, std::string>> currentOptions;
	size_t selectedIndex = 0;

	EditorApi *api = nullptr;
	FileExplorer *fileExplorer = nullptr;
	Settings *settings = nullptr;
};
