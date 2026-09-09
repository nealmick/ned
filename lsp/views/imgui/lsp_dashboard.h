#pragma once

#include "../../lsp_server_status.h"
#include "imgui.h"
#include <string>
#include <vector>

class FileExplorer;
class LSPClient;
class Settings;

class LSPDashboard
{
  public:
	LSPDashboard(LSPClient &client, FileExplorer &fileExplorer, Settings &settings);
	~LSPDashboard();

	// Main interface
	void render();
	void toggleShow() { show = !show; }
	void setShow(bool visible)
	{
		show = visible;
		if (visible)
		{
			refresh();
		}
	}
	bool isVisible() const { return show; }

	// Update server information
	void refresh();

  private:
	bool show = false;
	LSPClient *client = nullptr;
	FileExplorer *fileExplorer = nullptr;
	Settings *settings = nullptr;

	// Window properties (similar to settings)
	ImVec2 windowPos{300.0f, 200.0f};
	ImVec2 windowSize{800.0f, 500.0f};

	// Server information
	std::vector<LSPServerInfo> serverInfos;

	// Helper methods
	void renderServerList();
	void renderServerEntry(const LSPServerInfo &serverInfo);
	void handleWindowInput();
};
