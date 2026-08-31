#include <rendering/chunkGeometryOrder.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <random>
#include <vector>

namespace
{
	int pack(std::int16_t lower, std::int16_t upper)
	{
		return static_cast<int>(static_cast<std::uint32_t>(static_cast<std::uint16_t>(lower)) |
			(static_cast<std::uint32_t>(static_cast<std::uint16_t>(upper)) << 16U));
	}
}

int main()
{
	using mie::rendering::chunkGeometryPackedLess;
	using mie::rendering::chunkGeometrySortKey;

	assert(chunkGeometrySortKey(pack(-3, 7)) == std::make_pair(-3, 7));
	assert(chunkGeometrySortKey(pack(12, -9)) == std::make_pair(12, -9));

	std::mt19937 random(0x096U);
	std::uniform_int_distribution<std::uint32_t> distribution;
	std::vector<int> values;
	values.reserve(4096);
	for (int i = 0; i < 4096; ++i)
	{
		values.push_back(static_cast<int>(distribution(random)));
	}

	for (int value : values)
	{
		assert(!chunkGeometryPackedLess(value, value));
	}
	for (std::size_t i = 1; i < values.size(); ++i)
	{
		const int a = values[i - 1];
		const int b = values[i];
		assert(!(chunkGeometryPackedLess(a, b) && chunkGeometryPackedLess(b, a)));
	}

	std::sort(values.begin(), values.end(), chunkGeometryPackedLess);
	assert(std::is_sorted(values.begin(), values.end(), chunkGeometryPackedLess));
	for (std::size_t i = 2; i < values.size(); ++i)
	{
		if (chunkGeometryPackedLess(values[i - 2], values[i - 1]) &&
			chunkGeometryPackedLess(values[i - 1], values[i]))
		{
			assert(chunkGeometryPackedLess(values[i - 2], values[i]));
		}
	}

	return 0;
}
