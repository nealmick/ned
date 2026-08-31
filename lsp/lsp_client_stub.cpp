/*
	No-op LSPClient when NED_ENABLE_LSP=OFF.
	Same surface used by Ned/App/keybinds/settings; no lsp-framework.
*/

#include "lsp_client.h"

LSPClient::LSPClient(EditorApi &, FileExplorer &, Settings &) {}

LSPClient::~LSPClient() = default;

void LSPClient::setWorkspace(const std::string &) {}

bool LSPClient::init(const std::string &) { return false; }

void LSPClient::shutdown() {}

std::string LSPClient::getCurrentLanguage() const { return {}; }

const std::vector<LanguageServerInfo> &LSPClient::getLanguageServers() const
{
	static const std::vector<LanguageServerInfo> empty;
	return empty;
}

std::vector<std::string> LSPClient::getSupportedLanguages() const { return {}; }

void LSPClient::didOpen(const std::string &, const std::string &, int, const std::string &)
{
}

void LSPClient::didChange(const std::string &,
						  int,
						  const std::vector<EditorEvents::DocumentChange> &,
						  const FullTextProvider &)
{
}

void LSPClient::didSave(const std::string &, const FullTextProvider &) {}

void LSPClient::didClose(const std::string &) {}

LSPDiagnostics &LSPClient::diagnostics() { return diagnostics_; }

const LSPDiagnostics &LSPClient::diagnostics() const { return diagnostics_; }

bool LSPClient::isDocumentOpen(const std::string &) const { return false; }

bool LSPClient::keybinds() { return false; }

void LSPClient::bindEditorApi(EditorApi &) {}

void LSPClient::setHoverApi(EditorApi &) {}

void LSPClient::render() {}

bool LSPClient::startServer(const std::string &, const std::string &) { return false; }

void LSPClient::stopServer() {}

void LSPClient::initializeLanguageServers() {}

std::string LSPClient::expandEnvironmentVariables(const std::string &path) const
{
	return path;
}
