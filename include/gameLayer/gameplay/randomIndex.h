#pragma once

#include <cstddef>

template <class RandomEngine>
inline bool trySelectRandomIndex(RandomEngine &rng, std::size_t count, std::size_t &index)
{
	if (count == 0)
	{
		index = 0;
		return false;
	}

	index = static_cast<std::size_t>(rng()) % count;
	return true;
}
