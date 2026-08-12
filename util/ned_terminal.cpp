/*
	File: ned_terminal.cpp
	Description: Host wrapper for ImGui-Terminal (lib/imgui-terminal).
	Multi-session tab group inside the fixed bottom panel.
*/

#include "ned_terminal.h"

// terminal.h defines struct Font — keep it in this TU only.
#include "terminal.h"

#include "imgui.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct NedTerminal::Impl
{
	struct Session
	{
		Terminal term;
		bool started = false;
		int id = 0; // stable for ImGui tab ids
	};

	std::vector<std::unique_ptr<Session>> sessions;
	int active = 0;
	int nextId = 1;
	bool visible = true; // bottom panel on by default (VS Code-style)
	bool wantFocus = false;
	bool needsFontResync = false;
	float fontPx = 0.0f; // 0 = not set; applied to new sessions when host resyncs
	std::string projectRoot;

	Session *activeSession()
	{
		if (sessions.empty() || active < 0 || active >= static_cast<int>(sessions.size()))
			return nullptr;
		return sessions[static_cast<size_t>(active)].get();
	}

	void addSession()
	{
		auto s = std::make_unique<Session>();
		s->id = nextId++;
		sessions.push_back(std::move(s));
		active = static_cast<int>(sessions.size()) - 1;
	}

	void closeSession(int index)
	{
		if (index < 0 || index >= static_cast<int>(sessions.size()))
			return;
		Session &s = *sessions[static_cast<size_t>(index)];
		if (s.started)
		{
			s.term.shutdown();
			s.started = false;
		}
		sessions.erase(sessions.begin() + index);
		if (sessions.empty())
		{
			addSession();
			return;
		}
		if (active > index)
			--active;
		else if (active >= static_cast<int>(sessions.size()))
			active = static_cast<int>(sessions.size()) - 1;
	}

	bool ensureShell(Session &s)
	{
		if (s.started && s.term.is_alive())
			return true;

		if (s.started)
		{
			s.term.shutdown();
			s.started = false;
		}

		fs::path previous;
		std::error_code ec;
		bool restored = false;
		if (!projectRoot.empty())
		{
			previous = fs::current_path(ec);
			if (!ec)
			{
				fs::current_path(projectRoot, ec);
				if (ec)
					std::cerr << "[terminal] chdir to project root failed: "
							  << projectRoot << "\n";
				else
					restored = true;
			}
		}

		s.term.init(80, 24, nullptr);
		s.term.set_transparent(true);

		if (restored)
			fs::current_path(previous, ec);

		s.started = s.term.is_alive();
		if (!s.started)
		{
			std::cerr << "[terminal] shell failed to start\n";
			return false;
		}

		if (fontPx >= 6.0f)
			s.term.set_font_size(fontPx);
		else
			// load_fonts_once defaults to 16px — host should resync.
			needsFontResync = true;
		return true;
	}
};

NedTerminal::NedTerminal() : impl_(std::make_unique<Impl>()) { impl_->addSession(); }

NedTerminal::~NedTerminal() { shutdown(); }

void NedTerminal::setProjectRoot(const std::string &root) { impl_->projectRoot = root; }

bool NedTerminal::visible() const { return impl_->visible; }

bool NedTerminal::isStarted() const
{
	for (const auto &s : impl_->sessions)
	{
		if (s && s->started && s->term.is_alive())
			return true;
	}
	return false;
}

void NedTerminal::hide() { setVisible(false); }

void NedTerminal::shutdown()
{
	for (auto &s : impl_->sessions)
	{
		if (s && s->started)
		{
			s->term.shutdown();
			s->started = false;
		}
	}
	impl_->sessions.clear();
	impl_->active = 0;
	impl_->visible = false;
}

void NedTerminal::toggle() { setVisible(!impl_->visible); }

void NedTerminal::setVisible(bool on)
{
	const bool wasVisible = impl_->visible;
	impl_->visible = on;
	if (!impl_->visible)
		return;

	// Focus only on hide→show (not cold-start default-visible).
	if (!wasVisible)
		impl_->wantFocus = true;

	if (impl_->sessions.empty())
		impl_->addSession();

	if (Impl::Session *s = impl_->activeSession())
		impl_->ensureShell(*s);
}

bool NedTerminal::consumeNeedsFontResync()
{
	if (!impl_->needsFontResync)
		return false;
	impl_->needsFontResync = false;
	return true;
}

void NedTerminal::renderPanel()
{
	if (!impl_->visible)
		return;

	if (impl_->sessions.empty())
		impl_->addSession();

	// ---- Fixed tab group (not dockable, not reorderable) ----
	const ImGuiTabBarFlags tabFlags =
		ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_DrawSelectedOverline;
	if (ImGui::BeginTabBar("##ned_term_tabs", tabFlags))
	{
		// Trailing "+" — submit before items; always appears on the right.
		if (ImGui::TabItemButton(
				"+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
		{
			impl_->addSession();
			if (Impl::Session *s = impl_->activeSession())
				impl_->ensureShell(*s);
			impl_->wantFocus = true;
			impl_->needsFontResync = impl_->fontPx < 6.0f;
		}

		for (int i = 0; i < static_cast<int>(impl_->sessions.size());)
		{
			Impl::Session &s = *impl_->sessions[static_cast<size_t>(i)];
			char label[64];
			std::snprintf(label, sizeof(label), "Terminal %d###ned_term_%d", s.id, s.id);

			// Allow closing any tab when more than one exists.
			bool open = true;
			bool *pOpen = (impl_->sessions.size() > 1) ? &open : nullptr;
			if (ImGui::BeginTabItem(label, pOpen))
			{
				impl_->active = i;

				// Pump inactive shells so background jobs keep producing output.
				for (int j = 0; j < static_cast<int>(impl_->sessions.size()); ++j)
				{
					if (j == i)
						continue;
					Impl::Session &other = *impl_->sessions[static_cast<size_t>(j)];
					if (other.started && other.term.is_alive())
						other.term.tick();
				}

				if (!impl_->ensureShell(s))
				{
					ImGui::TextUnformatted(
						"Terminal session ended. Click + or press Cmd/Ctrl+T.");
				} else
				{
					if (impl_->wantFocus)
					{
						ImGui::SetKeyboardFocusHere();
						impl_->wantFocus = false;
					}
					s.term.draw_canvas();
				}
				ImGui::EndTabItem();
			}

			if (!open)
			{
				impl_->closeSession(i);
				continue;
			}
			++i;
		}
		ImGui::EndTabBar();
	}
}

void NedTerminal::reloadTerminalFonts(float desiredPx)
{
	if (desiredPx < 6.0f)
		desiredPx = 16.0f;
	impl_->fontPx = desiredPx;

	for (auto &sp : impl_->sessions)
	{
		if (!sp || !sp->started || !sp->term.is_alive())
			continue;
		const float cur = sp->term.get_font_size();
		// set_font_size no-ops when size is unchanged, but the atlas may have
		// been cleared by the host — force a rebuild by bumping then restoring.
		if (std::fabs(cur - desiredPx) < 0.01f)
			sp->term.set_font_size(desiredPx + 1.0f);
		sp->term.set_font_size(desiredPx);
	}
}
