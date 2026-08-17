#pragma once

#include <glm/vec3.hpp>

#include <cmath>

// Per-player v0.9 home/respawn state. It deliberately carries no block ID:
// the chosen home marker is gameplay/UI policy, while persistence only needs a
// safe world-space anchor. This keeps v0.7/v0.8 block IDs stable.
struct HomeRespawnState
{
	bool hasHome = false;
	glm::dvec3 position = {};

	void clear()
	{
		hasHome = false;
		position = {};
	}

	bool isValid() const
	{
		return hasHome &&
			std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(position.z) &&
			std::abs(position.x) <= 30'000'000.0 &&
			std::abs(position.y) <= 30'000'000.0 &&
			std::abs(position.z) <= 30'000'000.0;
	}

	bool set(glm::dvec3 newPosition)
	{
		HomeRespawnState candidate;
		candidate.hasHome = true;
		candidate.position = newPosition;
		if (!candidate.isValid()) { return false; }
		*this = candidate;
		return true;
	}
};
