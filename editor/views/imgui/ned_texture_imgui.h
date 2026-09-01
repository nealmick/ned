/*
	File: views/imgui/ned_texture_imgui.h
	Description: NedTextureId -> ImTextureID conversion for the ImGui backend.
*/

#pragma once

#include "../../platform/ned_types.h"
#include "imgui.h"

inline ImTextureID toImTexture(NedTextureId id)
{
	return ImTextureID(reinterpret_cast<void *>(id));
}
