#pragma once

#include <cstddef>
#include <limits>

namespace mie::rendering
{
	inline std::size_t growGpuBufferCapacity(std::size_t currentCapacity,
		std::size_t requiredCapacity)
	{
		if (requiredCapacity <= currentCapacity) { return currentCapacity; }
		std::size_t capacity = currentCapacity == 0u ? 256u : currentCapacity;
		while (capacity < requiredCapacity)
		{
			if (capacity > std::numeric_limits<std::size_t>::max() / 2u)
			{
				return requiredCapacity;
			}
			capacity *= 2u;
		}
		return capacity;
	}
}
