#include <gameplay/skinFormat.h>

#include <cassert>
#include <cstddef>
#include <iostream>
#include <vector>

namespace
{
	void setPixel(std::vector<unsigned char> &image, int width, int x, int y,
		unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255)
	{
		const std::size_t p = (static_cast<std::size_t>(y) * width + x) * 4;
		image[p + 0] = r;
		image[p + 1] = g;
		image[p + 2] = b;
		image[p + 3] = a;
	}

	void expectPixel(const std::vector<unsigned char> &image, int width, int x, int y,
		unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255)
	{
		const std::size_t p = (static_cast<std::size_t>(y) * width + x) * 4;
		assert(image[p + 0] == r);
		assert(image[p + 1] == g);
		assert(image[p + 2] == b);
		assert(image[p + 3] == a);
	}
}

int main()
{
	using namespace mie::skins;

	assert(detectSourceFormat(128, 128) == SourceFormat::Mie128);
	assert(detectSourceFormat(64, 64) == SourceFormat::Minecraft64);
	assert(detectSourceFormat(64, 32) == SourceFormat::MinecraftLegacy64x32);
	assert(detectSourceFormat(128, 64) == SourceFormat::MieLegacy128x64);
	assert(detectSourceFormat(32, 32) == SourceFormat::Invalid);
	assert(detectSourceFormat(256, 256) == SourceFormat::Invalid);

	std::vector<unsigned char> minecraft(64 * 64 * 4, 0);
	setPixel(minecraft, 64, 7, 9, 10, 20, 30, 40);
	auto modern = normalizeToMie(minecraft.data(), 64, 64);
	assert(modern);
	assert(modern.source == SourceFormat::Minecraft64);
	// Nearest-neighbour 2x conversion keeps each Minecraft texel as a 2x2 Mie texel block.
	expectPixel(modern.rgba, 128, 14, 18, 10, 20, 30, 40);
	expectPixel(modern.rgba, 128, 15, 19, 10, 20, 30, 40);

	std::vector<unsigned char> native(128 * 128 * 4, 0);
	setPixel(native, 128, 127, 127, 91, 92, 93, 94);
	auto mie = normalizeToMie(native.data(), 128, 128);
	assert(mie);
	assert(mie.source == SourceFormat::Mie128);
	expectPixel(mie.rgba, 128, 127, 127, 91, 92, 93, 94);

	std::vector<unsigned char> legacy(64 * 32 * 4, 0);
	setPixel(legacy, 64, 0, 16, 200, 10, 20, 255);
	setPixel(legacy, 64, 15, 16, 20, 10, 200, 255);
	setPixel(legacy, 64, 40, 16, 30, 210, 40, 255);
	setPixel(legacy, 64, 55, 16, 40, 30, 210, 255);
	auto oldMinecraft = normalizeToMie(legacy.data(), 64, 32);
	assert(oldMinecraft);
	assert(oldMinecraft.source == SourceFormat::MinecraftLegacy64x32);
	// The mirrored left-limb tiles are created in the lower half before the 2x upscale.
	expectPixel(oldMinecraft.rgba, 128, 32, 96, 20, 10, 200, 255);
	expectPixel(oldMinecraft.rgba, 128, 64, 96, 40, 30, 210, 255);

	std::vector<unsigned char> rows(2 * 2 * 4, 0);
	setPixel(rows, 2, 0, 0, 1, 2, 3, 4);
	setPixel(rows, 2, 0, 1, 5, 6, 7, 8);
	flipRows(rows, 2, 2);
	expectPixel(rows, 2, 0, 0, 5, 6, 7, 8);
	expectPixel(rows, 2, 0, 1, 1, 2, 3, 4);

	std::cout << "skin format tests passed\n";
	return 0;
}
