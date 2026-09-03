#pragma once

/*
	Language-server status probe shared by the backend dashboards (ImGui +
	Qt): one list describing every configured server — found path, running
	state — computed the same way the client resolves servers at startup.
*/

#include <string>
#include <vector>

class LSPClient;

struct LSPServerInfo
{
	std::string language;
	std::string serverPath;
	bool isFound = false;  // server executable was found on disk
	bool isActive = false; // server is currently running for its language
};

// Probe the client's lsp.json configuration against the filesystem plus the
// client's live state. UI-thread call (synchronous fs probe, a handful of
// servers).
std::vector<LSPServerInfo> probeLspServers(LSPClient &client);
