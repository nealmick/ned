#include "lsp_document_sync.h"
#include "../editor/services/diagnostics/diagnostics_store.h"
#include "../editor/util/doc_path.h"
#include "lsp_includes.h"
#include "lsp_trace.h"

#include <algorithm>

LSPDocumentSync::LSPDocumentSync(
	LSPDiagnostics &diagnostics,
	std::function<std::string(const std::string &)> detectLanguage)
	: diagnostics(diagnostics), detectLanguage(std::move(detectLanguage))
{
}

void LSPDocumentSync::connect(lsp::MessageHandler &h) { handler = &h; }

void LSPDocumentSync::disconnect()
{
	handler = nullptr;
	handshakeReady = false;
	notifyDidSave = false;
	saveIncludeText = false;
	syncKind = 2;
	const std::lock_guard<std::mutex> lock(stateMutex);
	pendingOpens.clear();
	openDocuments.clear();
}

void LSPDocumentSync::markHandshakeReady()
{
	handshakeReady = true;
	flushPending();
}

void LSPDocumentSync::applyCapabilities(const lsp::InitializeResult &result)
{
	syncKind = static_cast<int>(lsp::TextDocumentSyncKind::Incremental);
	notifyDidSave = false;
	saveIncludeText = false;

	if (!result.capabilities.textDocumentSync)
		return;

	const auto &sync = *result.capabilities.textDocumentSync;
	if (std::holds_alternative<lsp::TextDocumentSyncOptions>(sync))
	{
		const auto &opt = std::get<lsp::TextDocumentSyncOptions>(sync);
		if (opt.change)
			syncKind = static_cast<int>(opt.change->value());
		if (opt.save)
		{
			notifyDidSave = true;
			if (std::holds_alternative<lsp::SaveOptions>(*opt.save))
			{
				const auto &save = std::get<lsp::SaveOptions>(*opt.save);
				saveIncludeText = save.includeText.value_or(false);
			}
		}
	} else if (std::holds_alternative<lsp::TextDocumentSyncKindEnum>(sync))
	{
		syncKind =
			static_cast<int>(std::get<lsp::TextDocumentSyncKindEnum>(sync).value());
	}
}

bool LSPDocumentSync::trackedOpen(const std::string &key) const
{
	std::lock_guard<std::mutex> lock(stateMutex);
	return openDocuments.count(key) > 0;
}

void LSPDocumentSync::sendDidOpen(const std::string &key,
								  const std::string &content,
								  int version,
								  const std::string &languageId)
{
	if (!handler || key.empty())
		return;

	try
	{
		lsp::DidOpenTextDocumentParams params;
		params.textDocument.uri = lsp::Uri::fileUriFromPath(key);
		std::string lang = detectLanguage(key);
		if (lang.empty())
			lang = languageId.empty() ? "plaintext" : languageId;
		params.textDocument.languageId = std::move(lang);
		params.textDocument.version = version;
		params.textDocument.text = content;

		handler->sendNotification<lsp::notifications::TextDocument_DidOpen>(
			std::move(params));
		{
			std::lock_guard<std::mutex> lock(stateMutex);
			openDocuments.insert(key);
		}
		NED_LSP_TRACE("didOpen " << key << " lang=" << lang << " v=" << version
								 << " bytes=" << content.size());
	} catch (const std::exception &e)
	{
		std::cerr << "LSP: Failed to send didOpen: " << e.what() << std::endl;
	}
}

void LSPDocumentSync::upsertPendingOpen(const std::string &key,
										std::string content,
										int version,
										std::string languageId)
{
	std::lock_guard<std::mutex> lock(stateMutex);
	for (auto &doc : pendingOpens)
	{
		if (doc.key == key)
		{
			doc.content = std::move(content);
			doc.version = version;
			return;
		}
	}
	pendingOpens.push_back({key, std::move(content), std::move(languageId), version});
}

void LSPDocumentSync::flushPending()
{
	std::vector<PendingOpen> queued;
	{
		std::lock_guard<std::mutex> lock(stateMutex);
		queued.swap(pendingOpens);
	}
	for (auto &doc : queued)
	{
		if (trackedOpen(doc.key))
		{
			const std::string content = std::move(doc.content);
			didChange(doc.key, doc.version, {}, [&content] { return content; });
			continue;
		}
		sendDidOpen(doc.key, doc.content, doc.version, doc.languageId);
	}
}

void LSPDocumentSync::didOpen(const std::string &filePath,
							  const std::string &content,
							  int version,
							  const std::string &languageId)
{
	if (!handler || filePath.empty())
		return;

	const std::string key = DocPath::normalize(filePath);
	if (!handshakeReady)
	{
		upsertPendingOpen(key, content, version, languageId);
		return;
	}

	if (trackedOpen(key))
	{
		didChange(key, version, {}, [content] { return content; });
		return;
	}
	sendDidOpen(key, content, version, languageId);
}

void LSPDocumentSync::didChange(const std::string &filePath,
								int version,
								const std::vector<EditorEvents::DocumentChange> &changes,
								const FullTextProvider &fullText)
{
	if (!handler || filePath.empty())
		return;

	const std::string key = DocPath::normalize(filePath);
	if (!trackedOpen(key))
	{
		// Untracked: pre-handshake, or racing markHandshakeReady's flush
		// (handshakeReady flips before the queued didOpens are sent). Queue
		// the open carrying the latest text — never drop the edit — and
		// flush ourselves if the handshake already completed.
		upsertPendingOpen(key, fullText(), version);
		if (handshakeReady)
			flushPending();
		return;
	}
	if (syncKind == static_cast<int>(lsp::TextDocumentSyncKind::None))
		return;

	// Incremental servers with real change ranges never need the joined text —
	// invoking the provider is a full rope walk and must stay off the typing path.
	const bool incremental =
		syncKind == static_cast<int>(lsp::TextDocumentSyncKind::Incremental) &&
		!changes.empty();

	try
	{
		lsp::DidChangeTextDocumentParams params;
		params.textDocument.uri = lsp::Uri::fileUriFromPath(key);
		params.textDocument.version = version;

		if (incremental)
		{
			for (const auto &change : changes)
			{
				lsp::TextDocumentContentChangePartial event;
				event.range.start.line =
					static_cast<lsp::uint>(std::max(0, change.startLine));
				event.range.start.character =
					static_cast<lsp::uint>(std::max(0, change.startCharacter));
				event.range.end.line =
					static_cast<lsp::uint>(std::max(0, change.endLine));
				event.range.end.character =
					static_cast<lsp::uint>(std::max(0, change.endCharacter));
				event.text = change.text;
				params.contentChanges.push_back(std::move(event));
			}
		} else
		{
			lsp::TextDocumentContentChangeWholeDocument event;
			event.text = fullText();
			params.contentChanges.push_back(std::move(event));
		}

		handler->sendNotification<lsp::notifications::TextDocument_DidChange>(
			std::move(params));
		NED_LSP_TRACE("didChange " << filePath << " v=" << version
								   << " changes=" << changes.size()
								   << (incremental ? " (incr)" : " (full)"));
	} catch (const std::exception &e)
	{
		std::cerr << "LSP: Failed to send didChange: " << e.what() << std::endl;
	}
}

void LSPDocumentSync::didSave(const std::string &filePath,
							  const FullTextProvider &fullText)
{
	if (!handshakeReady || !handler || filePath.empty() || !notifyDidSave)
		return;
	const std::string key = DocPath::normalize(filePath);
	if (!trackedOpen(key))
		return;

	try
	{
		lsp::DidSaveTextDocumentParams params;
		params.textDocument.uri = lsp::Uri::fileUriFromPath(key);
		if (saveIncludeText)
			params.text = fullText();
		handler->sendNotification<lsp::notifications::TextDocument_DidSave>(
			std::move(params));
	} catch (const std::exception &e)
	{
		std::cerr << "LSP: Failed to send didSave: " << e.what() << std::endl;
	}
}

void LSPDocumentSync::didClose(const std::string &filePath)
{
	if (!handler || filePath.empty())
		return;

	const std::string key = DocPath::normalize(filePath);
	if (!trackedOpen(key))
		return;

	try
	{
		lsp::DidCloseTextDocumentParams params;
		params.textDocument.uri = lsp::Uri::fileUriFromPath(key);
		handler->sendNotification<lsp::notifications::TextDocument_DidClose>(
			std::move(params));
		NED_LSP_TRACE("didClose " << key);
	} catch (const std::exception &e)
	{
		std::cerr << "LSP: Failed to send didClose: " << e.what() << std::endl;
		// Drop tracking so we can re-open later even if the notify failed.
	}
	{
		std::lock_guard<std::mutex> lock(stateMutex);
		openDocuments.erase(key);
	}
	diagnostics.clear(key);
}

bool LSPDocumentSync::isDocumentOpen(const std::string &filePath) const
{
	if (filePath.empty())
		return false;
	return trackedOpen(DocPath::normalize(filePath));
}
