/*
	File: ned_embed.cpp
	Description: Thin embed facade over Workbench (Floating host mode).
*/

#include "ned_embed.h"

// When built as ned_embed without macOS window glue, provide a no-op so settings
// code that calls this does not fail to link.
extern "C" void updateMacOSWindowProperties(float opacity, bool blurEnabled)
{
	(void)opacity;
	(void)blurEnabled;
}

NedEmbed::NedEmbed()
{
	// Host must create ImGui context before constructing NedEmbed (same as before).
	initialize();
}

NedEmbed::~NedEmbed() { cleanup(); }

bool NedEmbed::initialize() { return workbench.initialize(WorkbenchHostMode::Floating); }

void NedEmbed::render() { workbench.render(); }

void NedEmbed::applySettingsChanges() { workbench.applySettings(); }

void NedEmbed::cleanup() { workbench.cleanup(); }
