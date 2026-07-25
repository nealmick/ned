/*
	No-op NedTerminal when NED_ENABLE_TERMINAL=OFF.
	Same public API as util/ned_terminal.cpp; no imgui-terminal link.
*/

#include "ned_terminal.h"

struct NedTerminal::Impl
{
	bool visible = false;
};

NedTerminal::NedTerminal() : impl_(std::make_unique<Impl>()) {}

NedTerminal::~NedTerminal() = default;

void NedTerminal::setProjectRoot(const std::string &) {}

bool NedTerminal::visible() const { return false; }

bool NedTerminal::isStarted() const { return false; }

void NedTerminal::hide() {}

void NedTerminal::shutdown() {}

void NedTerminal::toggle() {}

void NedTerminal::setVisible(bool) {}

void NedTerminal::renderFullscreen() {}

void NedTerminal::reloadTerminalFonts(float) {}

bool NedTerminal::consumeNeedsFontResync() { return false; }
