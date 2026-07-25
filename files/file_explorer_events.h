/*
	File: file_explorer_events.h
	Description: File explorer notifications. Owned by FileExplorer; composition
	root subscribes (e.g. LSP workspace / didOpen). Same pattern as EditorEvents.
*/

#pragma once

#include <functional>
#include <string>
#include <vector>

class FileExplorerEvents
{
  public:
	struct DidOpenProject
	{
		std::string root;
	};

	struct DidOpenDocument
	{
		std::string path;
	};

	// Buffer replaced or tab closed — previous path is no longer client-managed.
	struct DidCloseDocument
	{
		std::string path;
	};

	using DidOpenProjectFn = std::function<void(const DidOpenProject &)>;
	using DidOpenDocumentFn = std::function<void(const DidOpenDocument &)>;
	using DidCloseDocumentFn = std::function<void(const DidCloseDocument &)>;

	void subscribeDidOpenProject(DidOpenProjectFn fn);
	void subscribeDidOpenDocument(DidOpenDocumentFn fn);
	void subscribeDidCloseDocument(DidCloseDocumentFn fn);

	void emitDidOpenProject(const DidOpenProject &e);
	void emitDidOpenDocument(const DidOpenDocument &e);
	void emitDidCloseDocument(const DidCloseDocument &e);

	void clear();

  private:
	std::vector<DidOpenProjectFn> didOpenProjectListeners_;
	std::vector<DidOpenDocumentFn> didOpenDocumentListeners_;
	std::vector<DidCloseDocumentFn> didCloseDocumentListeners_;
};
