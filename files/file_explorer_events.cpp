/*
	File: file_explorer_events.cpp
	Description: File explorer notifications.
*/

#include "file_explorer_events.h"

void FileExplorerEvents::subscribeDidOpenProject(DidOpenProjectFn fn)
{
	if (fn)
		didOpenProjectListeners_.push_back(std::move(fn));
}

void FileExplorerEvents::subscribeDidOpenDocument(DidOpenDocumentFn fn)
{
	if (fn)
		didOpenDocumentListeners_.push_back(std::move(fn));
}

void FileExplorerEvents::subscribeDidCloseDocument(DidCloseDocumentFn fn)
{
	if (fn)
		didCloseDocumentListeners_.push_back(std::move(fn));
}

void FileExplorerEvents::emitDidOpenProject(const DidOpenProject &e)
{
	for (size_t i = 0; i < didOpenProjectListeners_.size(); ++i)
		didOpenProjectListeners_[i](e);
}

void FileExplorerEvents::emitDidOpenDocument(const DidOpenDocument &e)
{
	for (size_t i = 0; i < didOpenDocumentListeners_.size(); ++i)
		didOpenDocumentListeners_[i](e);
}

void FileExplorerEvents::emitDidCloseDocument(const DidCloseDocument &e)
{
	for (size_t i = 0; i < didCloseDocumentListeners_.size(); ++i)
		didCloseDocumentListeners_[i](e);
}

void FileExplorerEvents::clear()
{
	didOpenProjectListeners_.clear();
	didOpenDocumentListeners_.clear();
	didCloseDocumentListeners_.clear();
}
