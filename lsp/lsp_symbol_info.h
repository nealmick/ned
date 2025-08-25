#pragma once

#include "imgui.h"
#include <future>
#include <string>

class LSPSymbolInfo
{
  public:
	LSPSymbolInfo();
	~LSPSymbolInfo();

	// Check keybind and trigger symbol info if conditions are met
	void get();

	// Request hover information from LSP
	void
	request(int line, int character, std::function<void(const std::string &)> callback);

	// Render the symbol info UI
	void render();

  private:
	// State
	bool show;
	std::string symbolInfo;

	// Async request handling
	bool pending;
};

// Global instance
extern LSPSymbolInfo gLSPSymbolInfo;
