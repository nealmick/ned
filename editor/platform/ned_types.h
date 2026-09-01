/*
	File: platform/ned_types.h
	Description: Backend-neutral value types shared between the editor core
	and UI backends. No UI toolkit includes here — ImGui/Qt convert at their
	edge (see views/imgui/ned_color_imgui.h for the ImGui side).
*/

#pragma once

#include <cstdint>

struct NedVec2
{
	float x = 0.0f;
	float y = 0.0f;

	constexpr NedVec2() = default;
	constexpr NedVec2(float x_, float y_) : x(x_), y(y_) {}
};

struct NedColor
{
	float r = 1.0f;
	float g = 1.0f;
	float b = 1.0f;
	float a = 1.0f;

	constexpr NedColor() = default;
	constexpr NedColor(float r_, float g_, float b_, float a_ = 1.0f)
		: r(r_), g(g_), b(b_), a(a_)
	{
	}

	constexpr bool operator==(const NedColor &o) const
	{
		return r == o.r && g == o.g && b == o.b && a == o.a;
	}
};

// Opaque texture handle: an integer the backend interprets (ImGui: GL
// texture name via ImTextureID; Qt: resource cache key).
using NedTextureId = std::uintptr_t;
constexpr NedTextureId kNoTexture = 0;

// Packed 0xAABBGGRR (same layout as ImGui's ImU32 colors).
inline constexpr std::uint32_t nedColorToU32(const NedColor &c)
{
	const auto to8 = [](float v) {
		return static_cast<std::uint32_t>(v >= 1.0f ? 255 : v <= 0.0f ? 0 : v * 255.0f + 0.5f);
	};
	return (to8(c.a) << 24) | (to8(c.b) << 16) | (to8(c.g) << 8) | to8(c.r);
}
