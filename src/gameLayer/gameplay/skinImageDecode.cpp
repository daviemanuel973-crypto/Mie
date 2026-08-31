#include <gameplay/skinFormat.h>
#include <stb_image/stb_image.h>

#include <cstring>

unsigned char *mieLoadPlayerSkinImageFromMemory(const unsigned char *buffer, int len,
	int *x, int *y, int *channelsInFile, int desiredChannels)
{
	if (!buffer || len <= 0) { return nullptr; }

	int sourceWidth = 0;
	int sourceHeight = 0;
	int sourceChannels = 0;
	if (!stbi_info_from_memory(buffer, len, &sourceWidth, &sourceHeight, &sourceChannels) ||
		mie::skins::detectSourceFormat(sourceWidth, sourceHeight) == mie::skins::SourceFormat::Invalid)
	{
		return nullptr;
	}

	stbi_set_flip_vertically_on_load(false);
	unsigned char *decoded = stbi_load_from_memory(buffer, len, &sourceWidth,
		&sourceHeight, &sourceChannels, 4);
	// loadPlayerSkin historically requests bottom-up data. Restore that global
	// stb preference after our top-down decode; the normalized atlas is flipped
	// explicitly below.
	stbi_set_flip_vertically_on_load(true);
	if (!decoded) { return nullptr; }

	auto normalized = mie::skins::normalizeToMie(decoded, sourceWidth, sourceHeight);
	stbi_image_free(decoded);
	if (!normalized) { return nullptr; }

	mie::skins::flipRows(normalized.rgba, normalized.width, normalized.height);
	const std::size_t bytes = normalized.rgba.size();
	// Return memory through the same allocator contract used by STBI_FREE.
	// This repository configures stb_image with new[]/delete[], not malloc/free.
	auto *result = reinterpret_cast<unsigned char *>(STBI_MALLOC(bytes));
	if (!result) { return nullptr; }
	std::memcpy(result, normalized.rgba.data(), bytes);

	if (x) { *x = normalized.width; }
	if (y) { *y = normalized.height; }
	if (channelsInFile) { *channelsInFile = 4; }
	(void)desiredChannels;
	return result;
}
