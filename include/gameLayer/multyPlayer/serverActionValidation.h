#pragma once

#include <chunk.h>
#include <glm/vec3.hpp>

#include <cmath>
#include <cstdint>

namespace mie::serverValidation
{
	// The client raycasts blocks up to 20 units away. Keep a small server-side
	// tolerance for eye/feet origin differences and movement between packets.
	inline constexpr double MAX_BLOCK_ACTION_REACH = 22.5;
	inline constexpr std::int64_t MAX_WORLD_BLOCK_COORDINATE = 30'000'000;

	inline bool isWorldBlockPositionSane(const glm::ivec3 &position)
	{
		const std::int64_t x = position.x;
		const std::int64_t z = position.z;
		return position.y >= 0 && position.y < CHUNK_HEIGHT &&
			x >= -MAX_WORLD_BLOCK_COORDINATE && x <= MAX_WORLD_BLOCK_COORDINATE &&
			z >= -MAX_WORLD_BLOCK_COORDINATE && z <= MAX_WORLD_BLOCK_COORDINATE;
	}

	inline bool isBlockActionWithinReach(const glm::dvec3 &playerPosition,
		const glm::ivec3 &blockPosition,
		double maximumReach = MAX_BLOCK_ACTION_REACH)
	{
		if (!std::isfinite(playerPosition.x) || !std::isfinite(playerPosition.y) ||
			!std::isfinite(playerPosition.z) || maximumReach <= 0.0)
		{
			return false;
		}

		const glm::dvec3 blockCenter(
			static_cast<double>(blockPosition.x) + 0.5,
			static_cast<double>(blockPosition.y) + 0.5,
			static_cast<double>(blockPosition.z) + 0.5);
		const glm::dvec3 delta = blockCenter - playerPosition;
		const double distanceSquared = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
		return distanceSquared <= maximumReach * maximumReach;
	}

	inline bool isServerBlockActionPositionValid(const glm::dvec3 &playerPosition,
		const glm::ivec3 &blockPosition)
	{
		return isWorldBlockPositionSane(blockPosition) &&
			isBlockActionWithinReach(playerPosition, blockPosition);
	}
}
