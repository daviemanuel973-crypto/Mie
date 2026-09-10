#pragma once

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <vector>

namespace mie::rendering
{
	// Cache matrix indexes, never Chunk pointers. Unload/reload and culling still
	// use the current matrix on every frame. Only size/relative origin invalidate
	// distance order, including teleportation and render-distance changes.
	class ChunkDistanceOrder
	{
	public:
		const std::vector<int> &backToFront(int side, int originX, int originZ)
		{
			if (side < 1 || side > 102)
			{
				indices.clear();
				cachedSide = 0;
				return indices;
			}
			if (side == cachedSide && originX == cachedX && originZ == cachedZ)
			{
				return indices;
			}
			cachedSide = side;
			cachedX = originX;
			cachedZ = originZ;
			indices.resize(side * side);
			std::iota(indices.begin(), indices.end(), 0);
			auto distance = [=](int index)
			{
				// double avoids overflow even for an extreme, out-of-matrix origin.
				const double dx = static_cast<double>(index / side) - originX;
				const double dz = static_cast<double>(index % side) - originZ;
				return dx * dx + dz * dz;
			};
			std::sort(indices.begin(), indices.end(), [&](int a, int b)
			{
				const double da = distance(a), db = distance(b);
				return da != db ? da > db : a < b;
			});
			++rebuilds;
			return indices;
		}

		std::uint64_t rebuildCount() const { return rebuilds; }

	private:
		std::vector<int> indices;
		int cachedSide = 0, cachedX = 0, cachedZ = 0;
		std::uint64_t rebuilds = 0;
	};
}
