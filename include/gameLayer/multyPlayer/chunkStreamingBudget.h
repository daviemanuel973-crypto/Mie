#pragma once

#include <cstddef>

namespace mie::network
{
	constexpr std::size_t MAX_PENDING_CHUNK_PACKETS = 5u;

	inline bool canQueueAnotherChunkPacket(std::size_t pendingPackets)
	{
		return pendingPackets < MAX_PENDING_CHUNK_PACKETS;
	}
}
