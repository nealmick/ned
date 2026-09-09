#pragma once

/*
	Hover / diagnostic tooltip rendering. Parses the small markdown subset LSP
	hovers use (fenced code blocks, --- rules, prose) and renders it with the
	editor's syntax colors. Types come from the LSP response: plaintext is
	fenced at the source (lsp_symbol_info), so no code-sniffing here.
*/

#include "../../platform/lsp_editor.h"
#include "../../util/hover_markdown.h"

#include "imgui.h"

struct DiagnosticItem;

// Render parsed hover markdown (fences highlighted via tree-sitter snippets).
// Colors come through the LSPEditor seam — works for any backend.
void renderHoverMarkdown(const std::string &markdown,
						 LSPEditor &editor,
						 const std::string &fallbackLanguageId);

void renderDiagnosticTooltip(const std::vector<DiagnosticItem> &items,
							 class TooltipArbiter &arbiter);

// ImGui has one shared tooltip window per frame. The arbiter gives it a single
// explicit owner per frame — gutter marks, squiggle hover, or symbol hover —
// first claim wins, regardless of draw order. Owned by EditorFrame; exposed to
// the shell via the backend surface (EditorSurface::claimTooltip).
class TooltipArbiter
{
  public:
	bool claim();

  private:
	int claimedFrame = -1;
};
