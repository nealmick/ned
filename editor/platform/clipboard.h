/*
	File: platform/clipboard.h
	Description: System clipboard interface. The editor core edits against
	this; each backend installs an implementation (ImGui: Clipboard in
	editor/views/imgui/clipboard.h; Qt: QClipboard wrapper).
*/

#pragma once

#include <string>

class IClipboard
{
  public:
	virtual ~IClipboard() = default;

	virtual void setText(const std::string &text) = 0;
	// Empty string when the clipboard is empty or unavailable.
	virtual std::string text() const = 0;
};

// Registry used by EditorCommands. Backends install at startup
// (before the first editor is created); null means no clipboard.
void setEditorClipboard(IClipboard *clipboard);
IClipboard *editorClipboard();
