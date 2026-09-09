/*
	No-op TerminalPanel when NED_ENABLE_TERMINAL=OFF.
	Same public API as host/imgui/terminal_panel.cpp; no imgui-terminal link.
*/

#include "terminal_panel.h"

struct TerminalPanel::Impl
{
	bool visible = false;
};

TerminalPanel::TerminalPanel() : impl_(std::make_unique<Impl>()) {}

TerminalPanel::~TerminalPanel() = default;

void TerminalPanel::setProjectRoot(const std::string &) {}

bool TerminalPanel::visible() const { return false; }

bool TerminalPanel::isStarted() const { return false; }

void TerminalPanel::hide() {}

void TerminalPanel::shutdown() {}

void TerminalPanel::toggle() {}

void TerminalPanel::setVisible(bool, bool) {}

void TerminalPanel::renderPanel() {}

void TerminalPanel::applyFont(float) {}
float TerminalPanel::configuredFontPx() const { return 0.0f; }

bool TerminalPanel::consumeNeedsFontResync() { return false; }
