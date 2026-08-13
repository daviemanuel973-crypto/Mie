#define MIE_DISABLE_AMBIENT_SPAWN_REDIRECT 1
#include <multyPlayer/tick.h>

#include <algorithm>
#include <gameplay/serverSiegeRuntime.h>
#include <multyPlayer/enetServerFunction.h>

namespace
{
	bool isCloseToAnyPlayer(const glm::dvec3 &position)
	{
		for (const auto &entry : getAllClientsReff())
		{
			const glm::dvec2 delta(position.x - entry.second.playerData.entity.position.x,
				position.z - entry.second.playerData.entity.position.z);
			if (glm::dot(delta, delta) < 18.0 * 18.0) { return true; }
		}
		return false;
	}

	std::size_t ambientPopulation(const ServerChunkStorer &chunkManager)
	{
		std::size_t count = 0;
		for (const auto &chunkEntry : chunkManager.savedChunks)
		{
			if (!chunkEntry.second || !chunkEntry.second->otherData.withinSimulationDistance) { continue; }
			count += chunkEntry.second->entityData.pigs.size();
			count += chunkEntry.second->entityData.cats.size();
			count += chunkEntry.second->entityData.goblins.size();
		}
		return count;
	}

	bool spawnNeutralCommonGoblin(ServerChunkStorer &chunkManager, Goblin goblin,
		WorldSaver &worldSaver, std::minstd_rand &rng, std::uint64_t *spawnedId)
	{
		std::uint64_t id = 0;
		if (!spawnGoblin(chunkManager, goblin, worldSaver, rng, &id) || id == 0) { return false; }

		auto chunkPos = determineChunkThatIsEntityIn(goblin.position);
		auto *chunk = chunkManager.getChunkOrGetNull(chunkPos.x, chunkPos.y);
		if (!chunk) { return false; }

		auto found = chunk->entityData.goblins.find(id);
		if (found == chunk->entityData.goblins.end()) { return false; }

		auto &ambientGoblin = found->second;
		ambientGoblin.variant = GoblinServer::Common;
		ambientGoblin.variantConfigured = true;
		ambientGoblin.moveSpeedMultiplier = 1.f;
		ambientGoblin.entity.life.maxLife = 30;
		ambientGoblin.entity.life.life = std::min<short>(ambientGoblin.entity.life.life, 30);
		if (ambientGoblin.entity.life.life <= 0) { ambientGoblin.entity.life.life = 30; }
		ambientGoblin.basicEnemyBehaviour.playerLockedOn = 0;
		ambientGoblin.basicEnemyBehaviour.worriedTimer = 0.f;
		ambientGoblin.basicEnemyBehaviour.currentState = BasicEnemyBehaviour::stateStaying;

		if (spawnedId) { *spawnedId = id; }
		return true;
	}
}

bool spawnAmbientZombie(ServerChunkStorer &chunkManager, Zombie zombie, std::uint64_t newId)
{
	// Siege enemies are deliberate encounters. A close explicit/manual spawn is
	// preserved. The legacy ambient spawner always chooses distant positions,
	// so surprise zombies are rejected here.
	if (isServerSiegeWaveActive() || isCloseToAnyPlayer(zombie.position))
	{
		return spawnZombie(chunkManager, zombie, newId);
	}
	return false;
}

bool spawnAmbientGoblin(ServerChunkStorer &chunkManager, Goblin goblin,
	WorldSaver &worldSaver, std::minstd_rand &rng, std::uint64_t *spawnedId)
{
	if (spawnedId) { *spawnedId = 0; }

	// Deliberate close/manual spawns and siege spawns retain the normal variant
	// rules. Only distant peace-time spawns are converted into ambient ecology.
	if (isServerSiegeWaveActive() || isCloseToAnyPlayer(goblin.position))
	{
		return spawnGoblin(chunkManager, goblin, worldSaver, rng, spawnedId);
	}

	const std::size_t players = getAllClientsReff().size();
	const std::size_t cap = std::max<std::size_t>(6, players * 8);
	if (ambientPopulation(chunkManager) >= cap) { return false; }

	const int roll = getRandomNumber(rng, 0, 99);
	if (roll < 40)
	{
		return spawnNeutralCommonGoblin(chunkManager, goblin, worldSaver, rng, spawnedId);
	}
	if (roll < 82)
	{
		Pig pig;
		pig.position = goblin.position;
		pig.lastPosition = goblin.lastPosition;
		return spawnPig(chunkManager, pig, worldSaver, rng);
	}

	Cat cat;
	cat.position = goblin.position;
	cat.lastPosition = goblin.lastPosition;
	return spawnCat(chunkManager, cat, worldSaver, rng);
}
