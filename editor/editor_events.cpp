/*
	File: editor_events.cpp
	Description: Editor change notifications.
*/

#include "editor_events.h"

void EditorEvents::subscribeDidEdit(DidEditFn fn)
{
	if (fn)
		didEditListeners.push_back(std::move(fn));
}

void EditorEvents::subscribeDidSave(DidSaveFn fn)
{
	if (fn)
		didSaveListeners.push_back(std::move(fn));
}

void EditorEvents::subscribeDidRequestExclusiveOverlay(DidRequestExclusiveOverlayFn fn)
{
	if (fn)
		exclusiveOverlayListeners.push_back(std::move(fn));
}

void EditorEvents::emitDidEdit(const DidEdit &e)
{
	// Copy size in case a listener unsubscribes later (we don't support that yet).
	for (size_t i = 0; i < didEditListeners.size(); ++i)
		didEditListeners[i](e);
}

void EditorEvents::emitDidSave(const DidSave &e)
{
	for (size_t i = 0; i < didSaveListeners.size(); ++i)
		didSaveListeners[i](e);
}

void EditorEvents::emitDidRequestExclusiveOverlay(const DidRequestExclusiveOverlay &e)
{
	for (size_t i = 0; i < exclusiveOverlayListeners.size(); ++i)
		exclusiveOverlayListeners[i](e);
}

void EditorEvents::clear()
{
	didEditListeners.clear();
	didSaveListeners.clear();
	exclusiveOverlayListeners.clear();
}
