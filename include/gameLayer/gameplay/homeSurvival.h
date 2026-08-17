#pragma once

#include <glm/vec3.hpp>

#include <cmath>

struct ServerChunkStorer;
struct WorldSaver;

// Per-player v0.9 home/respawn state. It deliberately carries no block ID:
// the bedroll is a portable item and persistence only needs a safe world-space
// anchor. This keeps v0.7/v0.8 block IDs stable.
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

// Bedroll use is validated server-side against loaded collision data. The
// stored anchor is therefore known-safe at the moment it is created.
bool trySetHomeRespawn(HomeRespawnState &state, glm::dvec3 playerPosition,
	ServerChunkStorer &chunkStorer);

// If the home chunk is still loaded, revalidate/adjust the anchor. If it is
// unloaded, trust the previously validated anchor so a distant death can still
// return the player home; chunk streaming will then load that area normally.
glm::dvec3 resolveHomeOrWorldRespawn(const HomeRespawnState &state,
	ServerChunkStorer &chunkStorer, WorldSaver &worldSaver);
