#pragma once

/*
	Document synchronization (VSCode's TextDocumentContentSynchronizer):
	tracks open documents, negotiates sync kind / didSave from the server's
	InitializeResult, and converts editor change lists to LSP notifications.

	Pre-handshake opens are queued (refreshed by later edits) and flushed once
	the initialize reply arrives. didOpen/didClose are balanced per normalized
	path; re-open of an already-open path becomes didChange; didChange/didSave/
	didClose no-op for untracked paths.

	Called from the UI thread except flushPending() (message thread) — all
	state is mutex-guarded. FullTextProvider is invoked synchronously, only
	when the negotiated sync actually needs joined text (full sync, empty
	change list, saveIncludeText) so incremental edits never pay a rope join.
*/

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

#include "../editor/editor_events.h"

namespace lsp {

class MessageHandler;
struct InitializeResult;

} // namespace lsp

class LSPDiagnostics;

class LSPDocumentSync
{
  public:
	using FullTextProvider = std::function<std::string()>;

	// detectLanguage resolves a file path to a server language id ("" if
	// unknown) for didOpen params.
	LSPDocumentSync(LSPDiagnostics &diagnostics,
					std::function<std::string(const std::string &path)> detectLanguage);

	void connect(lsp::MessageHandler &handler); // server starting
	void disconnect();							// server stopping: reset all state
	void markHandshakeReady();					// initialize reply processed
	bool isReady() const { return handshakeReady; }

	// Sync kind / didSave negotiation from the server's capabilities.
	void applyCapabilities(const lsp::InitializeResult &result);

	void didOpen(const std::string &filePath,
				 const std::string &content,
				 int version,
				 const std::string &languageId);
	void didChange(const std::string &filePath,
				   int version,
				   const std::vector<EditorEvents::DocumentChange> &changes,
				   const FullTextProvider &fullText);
	void didSave(const std::string &filePath, const FullTextProvider &fullText);
	void didClose(const std::string &filePath);
	bool isDocumentOpen(const std::string &filePath) const;

  private:
	void sendDidOpen(const std::string &key,
					 const std::string &content,
					 int version,
					 const std::string &languageId);
	// Add or refresh the queued open for `key` (latest content wins); used by
	// both didOpen and untracked didChange.
	void upsertPendingOpen(const std::string &key,
						   std::string content,
						   int version,
						   std::string languageId = {});
	void flushPending();

	bool trackedOpen(const std::string &key) const;

	LSPDiagnostics &diagnostics;
	std::function<std::string(const std::string &)> detectLanguage;

	lsp::MessageHandler *handler = nullptr; // set by connect(), cleared by disconnect()
	std::atomic<bool> handshakeReady{false};

	std::atomic<int> syncKind{2}; // lsp::TextDocumentSyncKind::Incremental
	std::atomic<bool> notifyDidSave{false};
	std::atomic<bool> saveIncludeText{false};

	struct PendingOpen
	{
		std::string key; // normalized at enqueue — no re-canonicalizing per edit
		std::string content;
		std::string languageId;
		int version = 0;
	};
	mutable std::mutex stateMutex; // pendingOpens + openDocuments
	std::vector<PendingOpen> pendingOpens;
	std::unordered_set<std::string> openDocuments; // normalized keys
};
