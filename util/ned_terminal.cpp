/*
	File: ned_terminal.cpp
	Description: Host wrapper for ImGui-Terminal (lib/imgui-terminal).
*/

#include "ned_terminal.h"

// terminal.h defines struct Font — keep it in this TU only.
#include "terminal.h"

#include "imgui.h"

#include <cmath>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

struct NedTerminal::Impl
{
	Terminal term;
	bool visible = false;
	bool started = false;
	bool wantFocus = false;		  // set keyboard focus on next draw after open
	bool needsFontResync = false; // init loads 16px; host should match editor size
	std::string projectRoot;
};

NedTerminal::NedTerminal() : impl_(std::make_unique<Impl>()) {}

NedTerminal::~NedTerminal() { shutdown(); }

void NedTerminal::setProjectRoot(const std::string &root) { impl_->projectRoot = root; }

bool NedTerminal::visible() const { return impl_->visible; }

bool NedTerminal::isStarted() const { return impl_->started; }

void NedTerminal::hide() { setVisible(false); }

void NedTerminal::shutdown()
{
	if (impl_->started)
	{
		impl_->term.shutdown();
		impl_->started = false;
	}
	impl_->visible = false;
}

void NedTerminal::toggle() { setVisible(!impl_->visible); }

void NedTerminal::setVisible(bool on)
{
	impl_->visible = on;
	if (impl_->visible)
	{
		impl_->wantFocus = true; // focus canvas so typing works without a click
		// ensureStarted inline
		if (impl_->started && impl_->term.is_alive())
			return;

		if (impl_->started)
		{
			impl_->term.shutdown();
			impl_->started = false;
		}

		fs::path previous;
		std::error_code ec;
		bool restored = false;
		if (!impl_->projectRoot.empty())
		{
			previous = fs::current_path(ec);
			if (!ec)
			{
				fs::current_path(impl_->projectRoot, ec);
				if (ec)
					std::cerr << "[terminal] chdir to project root failed: "
							  << impl_->projectRoot << "\n";
				else
					restored = true;
			}
		}

		impl_->term.init(80, 24, nullptr);
		impl_->term.set_transparent(true);

		if (restored)
			fs::current_path(previous, ec);

		impl_->started = impl_->term.is_alive();
		if (!impl_->started)
			std::cerr << "[terminal] shell failed to start\n";
		else
			// load_fonts_once defaults to 16px — host must resync to editor size.
			impl_->needsFontResync = true;
	}
}

bool NedTerminal::consumeNeedsFontResync()
{
	if (!impl_->needsFontResync)
		return false;
	impl_->needsFontResync = false;
	return true;
}

void NedTerminal::renderFullscreen()
{
	if (!impl_->visible)
		return;

	// Restart shell if needed (setVisible already starts on open).
	if (!impl_->started || !impl_->term.is_alive())
	{
		if (impl_->visible)
			setVisible(true); // re-enter ensure path
	}

	if (!impl_->started || !impl_->term.is_alive())
	{
		ImGui::TextUnformatted("Terminal session ended. Press Cmd/Ctrl+T to restart.");
		return;
	}

	// Focus the InvisibleButton created inside draw_canvas (next item).
	if (impl_->wantFocus)
	{
		ImGui::SetKeyboardFocusHere();
		impl_->wantFocus = false;
	}
	impl_->term.draw_canvas();
}

void NedTerminal::reloadTerminalFonts(float desiredPx)
{
	if (!impl_->started)
		return;

	if (desiredPx < 6.0f)
		desiredPx = 16.0f;

	const float cur = impl_->term.get_font_size();
	// set_font_size no-ops when size is unchanged, but the atlas may have
	// been cleared by the host — force a rebuild by bumping then restoring.
	if (std::fabs(cur - desiredPx) < 0.01f)
		impl_->term.set_font_size(desiredPx + 1.0f);
	impl_->term.set_font_size(desiredPx);
}
