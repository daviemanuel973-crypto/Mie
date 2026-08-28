#include <gameplay/skinFormat.h>

#include <algorithm>
#include <cstring>

namespace mie::skins
{
	namespace
	{
		std::vector<unsigned char> copyImage(const unsigned char *rgba, int width, int height)
		{
			if (!rgba || width <= 0 || height <= 0) { return {}; }
			const std::size_t bytes = static_cast<std::size_t>(width) * height * 4;
			return std::vector<unsigned char>(rgba, rgba + bytes);
		}

		std::vector<unsigned char> resizeNearest(const std::vector<unsigned char> &source,
			int sourceWidth, int sourceHeight, int targetWidth, int targetHeight)
		{
			if (sourceWidth <= 0 || sourceHeight <= 0 || targetWidth <= 0 || targetHeight <= 0 ||
				source.size() < static_cast<std::size_t>(sourceWidth) * sourceHeight * 4)
			{
				return {};
			}

			std::vector<unsigned char> result(static_cast<std::size_t>(targetWidth) * targetHeight * 4);
			for (int y = 0; y < targetHeight; ++y)
			{
				const int sourceY = std::min(sourceHeight - 1, (y * sourceHeight) / targetHeight);
				for (int x = 0; x < targetWidth; ++x)
				{
					const int sourceX = std::min(sourceWidth - 1, (x * sourceWidth) / targetWidth);
					const std::size_t from = (static_cast<std::size_t>(sourceY) * sourceWidth + sourceX) * 4;
					const std::size_t to = (static_cast<std::size_t>(y) * targetWidth + x) * 4;
					std::memcpy(result.data() + to, source.data() + from, 4);
				}
			}
			return result;
		}

		void mirrorSquareRegion(const std::vector<unsigned char> &source, int sourceWidth,
			std::vector<unsigned char> &target, int targetWidth,
			int sourceX, int sourceY, int targetX, int targetY, int size)
		{
			for (int y = 0; y < size; ++y)
			{
				for (int x = 0; x < size; ++x)
				{
					const int mirroredX = size - 1 - x;
					const std::size_t from = (static_cast<std::size_t>(sourceY + y) * sourceWidth +
						(sourceX + mirroredX)) * 4;
					const std::size_t to = (static_cast<std::size_t>(targetY + y) * targetWidth +
						(targetX + x)) * 4;
					std::memcpy(target.data() + to, source.data() + from, 4);
				}
			}
		}

		std::vector<unsigned char> expandLegacy(const unsigned char *rgba, int width, int height)
		{
			if (!rgba || width <= 0 || height <= 0 || width != height * 2) { return {}; }

			const int outputSize = width;
			auto source = copyImage(rgba, width, height);
			std::vector<unsigned char> result(static_cast<std::size_t>(outputSize) * outputSize * 4, 0);

			// Preserve the complete legacy top half exactly. The modern atlas keeps
			// head, torso, right arm and right leg in these locations.
			for (int y = 0; y < height; ++y)
			{
				std::memcpy(result.data() + static_cast<std::size_t>(y) * outputSize * 4,
					source.data() + static_cast<std::size_t>(y) * width * 4,
					static_cast<std::size_t>(width) * 4);
			}

			// Minecraft <=1.7 stores only the right-side limbs. The exact per-face
			// orientation differs between individual faces, but mirroring the full
			// limb tile preserves the visible artwork and produces a deterministic,
			// usable left arm/leg for the Mie classic-proportion player model.
			const int scale = width / 64;
			mirrorSquareRegion(source, width, result, outputSize,
				0 * scale, 16 * scale, 16 * scale, 48 * scale, 16 * scale); // left leg
			mirrorSquareRegion(source, width, result, outputSize,
				40 * scale, 16 * scale, 32 * scale, 48 * scale, 16 * scale); // left arm

			return result;
		}
	}

	SourceFormat detectSourceFormat(int width, int height)
	{
		if (width == MIE_SKIN_SIZE && height == MIE_SKIN_SIZE) { return SourceFormat::Mie128; }
		if (width == MINECRAFT_SKIN_SIZE && height == MINECRAFT_SKIN_SIZE) { return SourceFormat::Minecraft64; }
		if (width == 64 && height == 32) { return SourceFormat::MinecraftLegacy64x32; }
		if (width == 128 && height == 64) { return SourceFormat::MieLegacy128x64; }
		return SourceFormat::Invalid;
	}

	NormalizedSkin normalizeToMie(const unsigned char *rgba, int width, int height)
	{
		NormalizedSkin result;
		result.source = detectSourceFormat(width, height);
		if (result.source == SourceFormat::Invalid || !rgba) { return result; }

		std::vector<unsigned char> working;
		switch (result.source)
		{
			case SourceFormat::Mie128:
				working = copyImage(rgba, width, height);
				break;
			case SourceFormat::Minecraft64:
				working = resizeNearest(copyImage(rgba, width, height), width, height,
					MIE_SKIN_SIZE, MIE_SKIN_SIZE);
				break;
			case SourceFormat::MinecraftLegacy64x32:
			{
				auto square = expandLegacy(rgba, width, height);
				working = resizeNearest(square, 64, 64, MIE_SKIN_SIZE, MIE_SKIN_SIZE);
				break;
			}
			case SourceFormat::MieLegacy128x64:
				working = expandLegacy(rgba, width, height);
				break;
			default:
				break;
		}

		if (working.size() != static_cast<std::size_t>(MIE_SKIN_SIZE) * MIE_SKIN_SIZE * 4)
		{
			result = {};
			return result;
		}

		result.width = MIE_SKIN_SIZE;
		result.height = MIE_SKIN_SIZE;
		result.rgba = std::move(working);
		return result;
	}

	void flipRows(std::vector<unsigned char> &rgba, int width, int height)
	{
		if (width <= 0 || height <= 0 ||
			rgba.size() < static_cast<std::size_t>(width) * height * 4)
		{
			return;
		}

		const std::size_t rowBytes = static_cast<std::size_t>(width) * 4;
		std::vector<unsigned char> row(rowBytes);
		for (int y = 0; y < height / 2; ++y)
		{
			unsigned char *top = rgba.data() + static_cast<std::size_t>(y) * rowBytes;
			unsigned char *bottom = rgba.data() + static_cast<std::size_t>(height - 1 - y) * rowBytes;
			std::memcpy(row.data(), top, rowBytes);
			std::memcpy(top, bottom, rowBytes);
			std::memcpy(bottom, row.data(), rowBytes);
		}
	}
}
