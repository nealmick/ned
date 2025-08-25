#pragma once
#include <map>
#include <string>
#include <vector>

class LSPUriOptions
{
  public:
	LSPUriOptions();
	~LSPUriOptions();

	void render(const std::string &title,
				const std::vector<std::map<std::string, std::string>> &options,
				bool &show);

  private:
	void handleSelection();

	std::string currentTitle;
	std::vector<std::map<std::string, std::string>> currentOptions;
	size_t selectedIndex;

	bool isEmbedded;
};

extern LSPUriOptions gLSPUriOptions;