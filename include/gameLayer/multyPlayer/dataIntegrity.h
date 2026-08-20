#pragma once

#include <chunk.h>
#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

namespace mie::dataIntegrity
{
	constexpr double StreamBoundaryEpsilon = 0.001;
	constexpr double MaximumWorldCoordinate = 30'000'000.0;
	constexpr double MaximumDroppedItemDistance = 8.0;

	inline glm::dvec3 clampEntityPositionToChunk(glm::dvec3 position, glm::ivec2 chunkPosition)
	{
		const double minX = static_cast<double>(chunkPosition.x) * CHUNK_SIZE + StreamBoundaryEpsilon;
		const double minZ = static_cast<double>(chunkPosition.y) * CHUNK_SIZE + StreamBoundaryEpsilon;
		const double maxX = static_cast<double>(chunkPosition.x + 1) * CHUNK_SIZE - StreamBoundaryEpsilon;
		const double maxZ = static_cast<double>(chunkPosition.y + 1) * CHUNK_SIZE - StreamBoundaryEpsilon;

		position.x = std::clamp(position.x, minX, maxX);
		position.z = std::clamp(position.z, minZ, maxZ);
		return position;
	}

	inline bool isFiniteWorldPosition(const glm::dvec3 &position)
	{
		return std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(position.z) &&
			std::abs(position.x) <= MaximumWorldCoordinate &&
			std::abs(position.y) <= MaximumWorldCoordinate &&
			std::abs(position.z) <= MaximumWorldCoordinate;
	}

	inline bool isDroppedItemSpawnPositionValid(const glm::dvec3 &playerPosition,
		const glm::dvec3 &dropPosition)
	{
		if (!isFiniteWorldPosition(playerPosition) || !isFiniteWorldPosition(dropPosition)) { return false; }

		const glm::dvec3 delta = dropPosition - playerPosition;
		return glm::dot(delta, delta) <= MaximumDroppedItemDistance * MaximumDroppedItemDistance;
	}
}
