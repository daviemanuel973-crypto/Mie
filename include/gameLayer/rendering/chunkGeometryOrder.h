#pragma once

#include <cstdint>
#include <utility>

namespace mie::rendering
{
	constexpr int signed16(std::uint32_t value)
	{
		value &= 0xffffU;
		return value <= 0x7fffU ? static_cast<int>(value) : static_cast<int>(value) - 0x10000;
	}

	constexpr std::pair<int, int> chunkGeometrySortKey(int packed)
	{
		const auto bits = static_cast<std::uint32_t>(packed);
		return {signed16(bits), signed16(bits >> 16U)};
	}

	constexpr bool chunkGeometryPackedLess(int a, int b)
	{
		return chunkGeometrySortKey(a) < chunkGeometrySortKey(b);
	}
}
