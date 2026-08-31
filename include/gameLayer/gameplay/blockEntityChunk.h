#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <metrics.h>

namespace mie::clientEntities
{
	inline bool blockPositionBelongsToChunk(const glm::ivec3 &blockPosition,
		const glm::ivec2 &chunkPosition)
	{
		return divideChunk(blockPosition.x) == chunkPosition.x &&
			divideChunk(blockPosition.z) == chunkPosition.y;
	}
}
