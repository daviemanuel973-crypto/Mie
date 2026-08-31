#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace mie::skins
{
	constexpr int MIE_SKIN_SIZE = 128;
	constexpr int MINECRAFT_SKIN_SIZE = 64;

	enum class SourceFormat : std::uint8_t
	{
		Invalid = 0,
		Mie128,
		Minecraft64,
		MinecraftLegacy64x32,
		MieLegacy128x64,
	};

	struct NormalizedSkin
	{
		SourceFormat source = SourceFormat::Invalid;
		int width = 0;
		int height = 0;
		std::vector<unsigned char> rgba;

		explicit operator bool() const
		{
			return source != SourceFormat::Invalid && width == MIE_SKIN_SIZE &&
				height == MIE_SKIN_SIZE &&
				rgba.size() == static_cast<std::size_t>(MIE_SKIN_SIZE * MIE_SKIN_SIZE * 4);
		}
	};

	SourceFormat detectSourceFormat(int width, int height);

	// Mie Skin v1 is a 128x128 RGBA atlas using the same normalized UV layout
	// as the modern 64x64 Minecraft Java skin. 64x64 Minecraft skins are
	// nearest-neighbour scaled to 128x128. Legacy 64x32/128x64 skins are first
	// expanded to the modern square layout and then normalized.
	NormalizedSkin normalizeToMie(const unsigned char *rgba, int width, int height);

	// OpenGL in the existing renderer expects player textures in bottom-up row
	// order. Keep this separate so the format conversion itself stays testable
	// and uses the conventional top-left image coordinate system.
	void flipRows(std::vector<unsigned char> &rgba, int width, int height);
}
