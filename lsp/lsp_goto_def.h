#pragma once

#include "imgui.h"
#include <functional>
#include <string>

class LSPGotoDef
{
  public:
	LSPGotoDef();
	~LSPGotoDef();

	// Check keybind and trigger goto definition if conditions are met
	void get();

	// Render the goto definition UI
	void render();

  private:
	// State
	bool show;
	std::string gotoInfo;

	// Async request handling
	bool pending;
};

// Global instance
extern LSPGotoDef gLSPGotoDef;