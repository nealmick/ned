/*
	File: lsp_editor.h
	Description: The editor seam LSP core consumes — caret/path/line queries,
	a deferred jump, and theme colors (hover markdown rendering). Implemented
	per backend (ImGui: EditorApi; Qt: QtEditorView) so LSPClient / LSPGoto /
	LSPHover stay UI-free. Columns are UTF-8 byte offsets; the UTF-16
	conversion happens in the lsp module.
*/

#pragma once

#include "../services/highlight/capture_map.h"
#include "ned_types.h"

#include <string>

class LspEditor
{
  public:
	virtual ~LspEditor() = default;

	virtual void getCaret(int &row, int &column) const = 0;
	virtual std::string line(int row) const = 0;
	virtual const std::string &path() const = 0;
	virtual const std::string &languageId() const = 0;
	// Deferred caret place + center (post-layout goto within this document).
	virtual void requestCursorCenter(int row, int column) = 0;

	// Theme colors: default text + per-slot syntax colors (the hover tooltip
	// renders code snippets through these, ImGui parity).
	virtual NedColor defaultTextColor() const = 0;
	virtual NedColor syntaxColor(ThemeSlot slot) const = 0;
};
