/*
	File: ned_terminal.h
	Description: Thin host wrapper around lib/imgui-terminal.

	Cmd/Ctrl+T toggles a fixed bottom panel under the editor dock (not a
	dockable window; workbench owns the split). Default visible.

	Multiple shell sessions live in a non-reorderable, non-dockable tab
	group with a trailing "+" to spawn another. Sessions are not ImGui
	dock nodes.

	Implementation is pimpl'd so terminal.h (which defines its own Font) is
	not visible to translation units that also use util/font.h.
*/

#pragma once

#include <memory>
#include <string>

class NedTerminal
{
  public:
	NedTerminal();
	~NedTerminal();

	NedTerminal(const NedTerminal &) = delete;
	NedTerminal &operator=(const NedTerminal &) = delete;

	// Working directory for the next shell spawn (project root).
	void setProjectRoot(const std::string &root);

	bool visible() const;
	bool isStarted() const;

	// Toggle visibility. Saves editor on open (caller should also save).
	void toggle();
	void setVisible(bool on);
	void hide();

	// Draw into the current ImGui region (bottom panel host child).
	void renderPanel();

	// Tear down all PTYs. Safe to call multiple times.
	void shutdown();

	// After the host clears/rebuilds the ImGui font atlas: re-add terminal
	// fonts at `desiredPx` on every live session. Caller must then re-merge
	// host editor fonts with Font::load(/*clearAtlas=*/false).
	void reloadTerminalFonts(float desiredPx);

	// True after shell init (default 16px fonts) until the host applies
	// editor font size via reloadTerminalFonts + Font::load(false).
	bool consumeNeedsFontResync();

  private:
	struct Impl;
	std::unique_ptr<Impl> impl_;
};
