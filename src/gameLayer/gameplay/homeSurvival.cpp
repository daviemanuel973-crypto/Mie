#include <gameplay/homeSurvival.h>

#include <multyPlayer/server.h>
#include <multyPlayer/serverChunkStorer.h>
#include <chunk.h>

#include <algorithm>
#include <cmath>

namespace
{
	bool safeFeetPosition(ServerChunkStorer &chunkStorer, const glm::ivec3 &feet)
	{
		Block *ground = chunkStorer.getBlockSafe(feet + glm::ivec3(0, -1, 0));
		Block *feetBlock = chunkStorer.getBlockSafe(feet);
		Block *headBlock = chunkStorer.getBlockSafe(feet + glm::ivec3(0, 1, 0));
		return ground && feetBlock && headBlock && ground->isColidable() &&
			!feetBlock->isColidable() && !headBlock->isColidable() &&
			ground->getType() != BlockTypes::water;
	}

	bool findSafeNear(ServerChunkStorer &chunkStorer, glm::dvec3 around,
		glm::dvec3 &result)
	{
		const glm::ivec3 origin = from3DPointToBlock(around);
		for (int radius = 0; radius <= 2; ++radius)
		{
			for (int dy = -2; dy <= 2; ++dy)
			{
				for (int dz = -radius; dz <= radius; ++dz)
				{
					for (int dx = -radius; dx <= radius; ++dx)
					{
						if (radius != 0 && std::max(std::abs(dx), std::abs(dz)) != radius)
						{
							continue;
						}
						const glm::ivec3 candidate = origin + glm::ivec3(dx, dy, dz);
						if (candidate.y <= 0 || candidate.y >= CHUNK_HEIGHT - 1) { continue; }
						if (safeFeetPosition(chunkStorer, candidate))
						{
							result = glm::dvec3(candidate);
							return true;
						}
					}
				}
			}
		}
		return false;
	}
}

bool trySetHomeRespawn(HomeRespawnState &state, glm::dvec3 playerPosition,
	ServerChunkStorer &chunkStorer)
{
	glm::dvec3 safePosition;
	if (!findSafeNear(chunkStorer, playerPosition, safePosition)) { return false; }
	return state.set(safePosition);
}

glm::dvec3 resolveHomeOrWorldRespawn(const HomeRespawnState &state,
	ServerChunkStorer &chunkStorer, WorldSaver &worldSaver)
{
	if (!state.isValid()) { return resolveSafeServerSpawn(worldSaver); }

	const glm::ivec3 homeBlock = from3DPointToBlock(state.position);
	const auto *loadedHomeChunk = chunkStorer.getChunkOrGetNull(
		divideChunk(homeBlock.x), divideChunk(homeBlock.z));
	if (!loadedHomeChunk)
	{
		// The anchor was validated when the bedroll was used. Trust it while its
		// chunk is unloaded so deaths far from home still return to the base.
		return state.position;
	}

	glm::dvec3 safePosition;
	if (findSafeNear(chunkStorer, state.position, safePosition)) { return safePosition; }
	return resolveSafeServerSpawn(worldSaver);
}
