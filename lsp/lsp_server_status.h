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

// Toolkit-neutral status color roles — each backend maps them to its own
// palette (they are NOT colors).
enum class LSPStatusColorRole {
	Positive, // server found / running
	Negative, // server missing
	Muted,	  // found but not running
	Dim		  // not applicable
};

struct LSPServerStatus
{
	const char *text = "";
	LSPStatusColorRole color = LSPStatusColorRole::Dim;
};

// "● Found" / "● Missing" for the executable probe column.
inline LSPServerStatus lspFoundStatus(const LSPServerInfo &server)
{
	return server.isFound ? LSPServerStatus{"● Found", LSPStatusColorRole::Positive}
						  : LSPServerStatus{"● Missing", LSPStatusColorRole::Negative};
}

// "● Active" / "● Inactive" / "N/A" for the running-state column.
inline LSPServerStatus lspActiveStatus(const LSPServerInfo &server)
{
	if (!server.isFound)
		return LSPServerStatus{"N/A", LSPStatusColorRole::Dim};
	return server.isActive ? LSPServerStatus{"● Active", LSPStatusColorRole::Positive}
						   : LSPServerStatus{"● Inactive", LSPStatusColorRole::Muted};
}
