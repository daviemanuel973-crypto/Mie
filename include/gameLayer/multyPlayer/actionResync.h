#pragma once

#include <cstdint>

namespace mie::network
{
	constexpr std::uint64_t ACTION_RESYNC_TIMEOUT_MS = 20'000;
	constexpr std::uint64_t ACTION_RESYNC_RETRY_MS = 5'000;

	inline bool shouldRequestActionResync(std::uint64_t now,
		std::uint64_t oldestEventTime, std::uint64_t lastRequestTime)
	{
		if (now < oldestEventTime || now - oldestEventTime < ACTION_RESYNC_TIMEOUT_MS)
		{
			return false;
		}
		return lastRequestTime == 0 ||
			(now >= lastRequestTime && now - lastRequestTime >= ACTION_RESYNC_RETRY_MS);
	}
}
