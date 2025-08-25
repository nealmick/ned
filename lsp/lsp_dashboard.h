#pragma once

#include "imgui.h"
#include <map>
#include <string>
#include <vector>

struct LSPServerInfo
{
	std::string language;
	std::string serverPath;
	bool isFound;		// Whether server executable was found
	bool isActive;		// Whether server is currently running
	std::string status; // Additional status information
};

class LSPDashboard
{
  public:
	LSPDashboard();
	~LSPDashboard();

	// Main interface
	void render();
	void toggleShow() { show = !show; }
	void setShow(bool visible)
	{
		show = visible;
		if (visible)
		{
			refreshServerInfo();
		}
	}
	bool isVisible() const { return show; }

	// Update server information
	void refreshServerInfo();

  private:
	bool show = false;

	// Window properties (similar to settings)
	ImVec2 windowPos{300.0f, 200.0f};
	ImVec2 windowSize{800.0f, 500.0f};

	// Server information
	std::vector<LSPServerInfo> serverInfos;

	// Helper methods
	void renderServerList();
	void renderServerEntry(const LSPServerInfo &serverInfo);
	void updateServerStatus();
	std::vector<std::string> getSupportedLanguages();
	void handleWindowInput();
};

// Global instance declaration
extern LSPDashboard gLSPDashboard;