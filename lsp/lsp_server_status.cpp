#include "lsp_server_status.h"
#include "lsp_client.h"

std::vector<LSPServerInfo> probeLspServers(LSPClient &client)
{
	std::vector<LSPServerInfo> out;

	// Same resolution the client itself uses at startup (findServerPath):
	// the first configured path that exists and is a regular file wins —
	// one implementation, so the dashboard can never disagree with what
	// the client would actually launch.
	for (const auto &serverConfig : client.getLanguageServers())
	{
		LSPServerInfo info;
		info.language = serverConfig.language;

		const std::string serverPath = client.findServerPath(serverConfig.language);
		info.serverPath = serverPath.empty() ? "Not found" : serverPath;
		info.isFound = !serverPath.empty();

		// Active = found + client running + running for this language.
		info.isActive = info.isFound && client.isInitialized() &&
						client.getCurrentLanguage() == serverConfig.language;

		out.push_back(std::move(info));
	}
	return out;
}
