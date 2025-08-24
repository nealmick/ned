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
	std::string currentTitle;
	std::vector<std::map<std::string, std::string>> currentOptions;
};

extern LSPUriOptions gLSPUriOptions;