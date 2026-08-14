#include <gameplay/serverSiegeRuntime.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <multyPlayer/serverChunkStorer.h>
#include <gameplay/allentities.h>
#include <gameplay/basicEnemyBehaviour.h>
#include <gameplay/goblin.h>
#include <gameplay/worldTime.h>
#include <gameplay/zombie.h>
#include <multyPlayer/client.h>
#include <multyPlayer/chunkSaver.h>
#include <multyPlayer/enetServerFunction.h>
#include <multyPlayer/packet.h>
#include <multyPlayer/tick.h>
#include <platformTools.h>
#include <safeSave.h>

namespace
{
	struct Ivec3Hash
	{
		std::size_t operator()(const glm::ivec3 &value) const noexcept
		{
			std::size_t result = static_cast<std::size_t>(value.x) * 73856093u;
			result ^= static_cast<std::size_t>(value.y) * 19349663u;
			result ^= static_cast<std::size_t>(value.z) * 83492791u;
			return result;
		}
	};

	struct ServerSiegeState
	{
		ServerSiegeState(): director(SiegeTuning{}) {}

		SiegeDirector director;
		WorldCycleClock worldClock;
		std::unordered_set<std::uint64_t> activeEnemyIds;
		std::unordered_map<glm::ivec3, float, Ivec3Hash> defenseDamage;
		float spawnTimer = 0.f;
		float defenseTimer = 0.f;
		float trapTimer = 0.f;
		float statusBroadcastTimer = 0.f;
		float worldTimeBroadcastTimer = 0.f;
		SiegeStatus lastBroadcastStatus{};
		bool hasBroadcastStatus = false;
		bool naturalSiegeActive = false;
		std::uint64_t naturalSiegeDay = 0;
	};

	ServerSiegeState state;

	std::string worldProgressPath(const WorldSaver &worldSaver)
	{
		return (std::filesystem::path(worldSaver.savePath) / "worldProgress").string();
	}

	glm::ivec3 toBlockPosition(const glm::dvec3 &position)
	{
		return glm::ivec3(glm::floor(position + glm::dvec3(0.5)));
	}

	bool unsuitableGround(BlockType type)
	{
		return type == BlockTypes::water || type == BlockTypes::ice ||
			isAnyLeaves(type) || isAnyWoddenLOG(type) || isDecorativeFurniture(type);
	}

	bool farEnoughFromPlayers(const glm::dvec3 &position)
	{
		for (const auto &entry : getAllClientsReff())
		{
			const glm::dvec2 delta(position.x - entry.second.playerData.entity.position.x,
				position.z - entry.second.playerData.entity.position.z);
			if (glm::dot(delta, delta) < 18.0 * 18.0) { return false; }
		}
		return true;
	}

	bool findSiegeSpawn(ServerChunkStorer &chunkStorer, const glm::dvec3 &target,
		std::minstd_rand &rng, glm::dvec3 &spawnPosition)
	{
		constexpr float pi = 3.14159265358979323846f;
		for (int attempt = 0; attempt < 24; ++attempt)
		{
			const float angle = getRandomNumberFloat(rng, 0.f, pi * 2.f);
			const float distance = getRandomNumberFloat(rng, 24.f, 36.f);
			const int worldX = static_cast<int>(std::floor(target.x + std::cos(angle) * distance));
			const int worldZ = static_cast<int>(std::floor(target.z + std::sin(angle) * distance));

			auto *chunk = chunkStorer.getChunkOrGetNull(divideChunk(worldX), divideChunk(worldZ));
			if (!chunk || !chunk->otherData.withinSimulationDistance) { continue; }

			const int localX = modBlockToChunk(worldX);
			const int localZ = modBlockToChunk(worldZ);
			for (int y = CHUNK_HEIGHT - 3; y >= 2; --y)
			{
				auto *ground = chunk->chunk.safeGet(localX, y, localZ);
				auto *feet = chunk->chunk.safeGet(localX, y + 1, localZ);
				auto *head = chunk->chunk.safeGet(localX, y + 2, localZ);
				if (!ground || !feet || !head) { continue; }
				if (!ground->isColidable() || unsuitableGround(ground->getType())) { continue; }
				if (!feet->air() || !head->air()) { continue; }

				spawnPosition = glm::dvec3(worldX, y + 0.51, worldZ);
				if (farEnoughFromPlayers(spawnPosition)) { return true; }
				break;
			}
		}
		return false;
	}

	std::vector<std::uint64_t> survivalPlayers()
	{
		std::vector<std::uint64_t> result;
		for (const auto &entry : getAllClientsReff())
		{
			if (!entry.second.playerData.killed &&
				entry.second.playerData.otherPlayerSettings.gameMode == OtherPlayerSettings::SURVIVAL)
			{
				result.push_back(entry.first);
			}
		}
		return result;
	}

	bool spawnOneSiegeEnemy(ServerChunkStorer &chunkStorer, WorldSaver &worldSaver,
		std::minstd_rand &rng, const std::vector<std::uint64_t> &players)
	{
		if (players.empty()) { return false; }
		const std::uint64_t targetId = players[static_cast<std::size_t>(
			getRandomNumber(rng, 0, static_cast<int>(players.size()) - 1))];
		auto foundTarget = getAllClientsReff().find(targetId);
		if (foundTarget == getAllClientsReff().end()) { return false; }

		glm::dvec3 spawnPosition;
		if (!findSiegeSpawn(chunkStorer, foundTarget->second.playerData.getPosition(), rng, spawnPosition))
		{
			return false;
		}

		if (getRandomChance(rng, 0.72f))
		{
			Zombie zombie;
			zombie.position = spawnPosition;
			zombie.lastPosition = spawnPosition;
			const std::uint64_t newId = getEntityIdAndIncrement(worldSaver, EntityType::zombies);
			if (!spawnZombie(chunkStorer, zombie, newId)) { return false; }

			auto *chunk = chunkStorer.getChunkOrGetNull(
				divideChunk(spawnPosition.x), divideChunk(spawnPosition.z));
			if (chunk)
			{
				auto found = chunk->entityData.zombies.find(newId);
				if (found != chunk->entityData.zombies.end()) { found->second.forceTarget(targetId); }
			}
			state.activeEnemyIds.insert(newId);
			return true;
		}

		Goblin goblin;
		goblin.position = spawnPosition;
		goblin.lastPosition = spawnPosition;
		std::uint64_t newId = 0;
		if (!spawnGoblin(chunkStorer, goblin, worldSaver, rng, &newId) || newId == 0) { return false; }

		auto *chunk = chunkStorer.getChunkOrGetNull(
			divideChunk(spawnPosition.x), divideChunk(spawnPosition.z));
		if (chunk)
		{
			auto found = chunk->entityData.goblins.find(newId);
			if (found != chunk->entityData.goblins.end()) { found->second.forceTarget(targetId); }
		}
		state.activeEnemyIds.insert(newId);
		return true;
	}

	void pruneMissingSiegeEnemies(ServerChunkStorer &chunkStorer)
	{
		for (auto it = state.activeEnemyIds.begin(); it != state.activeEnemyIds.end();)
		{
			if (!chunkStorer.getEntityPosition(*it).has_value()) { it = state.activeEnemyIds.erase(it); }
			else { ++it; }
		}
	}

	void removeRemainingNaturalSiegeEnemies(ServerChunkStorer &chunkStorer,
		WorldSaver &worldSaver)
	{
		const auto ids = state.activeEnemyIds;
		for (const std::uint64_t entityId : ids)
		{
			if (chunkStorer.removeEntity(worldSaver, entityId))
			{
				Packet packet;
				packet.header = headerRemoveEntity;
				Packet_RemoveEntity data;
				data.EID = entityId;
				broadCast(packet, &data, sizeof(data), nullptr, true, channelEntityPositions);
			}
		}
		state.activeEnemyIds.clear();
		state.defenseDamage.clear();
		state.director.cancelCurrentSiege();
		state.naturalSiegeActive = false;
		state.naturalSiegeDay = 0;
	}

	bool isDefenseBlock(BlockType type)
	{
		return type == BlockTypes::reinforcedBarricade || type == BlockTypes::wooden_wall ||
			type == BlockTypes::wooden_plank || type == BlockTypes::logWall;
	}

	float defenseDurability(BlockType type)
	{
		if (type == BlockTypes::reinforcedBarricade) { return 100.f; }
		if (type == BlockTypes::logWall) { return 70.f; }
		if (type == BlockTypes::wooden_plank) { return 50.f; }
		return 40.f;
	}

	void broadcastBlockDestroyed(glm::ivec3 position, const Block &block)
	{
		Packet packet;
		packet.header = headerPlaceBlocks;
		Packet_PlaceBlocks data;
		data.blockPos = position;
		data.blockInfo = block;
		broadCast(packet, &data, sizeof(data), nullptr, true, channelChunksAndBlocks);
	}

	void updateBarricadeDamage(float elapsed, ServerChunkStorer &chunkStorer)
	{
		std::unordered_set<glm::ivec3, Ivec3Hash> touched;
		const glm::ivec3 directions[] = {
			{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1},
			{1, 1, 0}, {-1, 1, 0}, {0, 1, 1}, {0, 1, -1},
		};

		for (const std::uint64_t enemyId : state.activeEnemyIds)
		{
			const auto position = chunkStorer.getEntityPosition(enemyId);
			if (!position) { continue; }
			const glm::ivec3 base = toBlockPosition(*position);

			for (const glm::ivec3 &direction : directions)
			{
				const glm::ivec3 candidate = base + direction;
				Block *block = chunkStorer.getBlockSafe(candidate);
				if (!block || !isDefenseBlock(block->getType())) { continue; }

				touched.insert(candidate);
				float &damage = state.defenseDamage[candidate];
				damage += 9.f * elapsed;
				if (damage >= defenseDurability(block->getType()))
				{
					SavedChunk *chunk = nullptr;
					block = chunkStorer.getBlockSafeAndChunk(candidate, chunk);
					if (block && chunk && isDefenseBlock(block->getType()))
					{
						*block = {};
						chunk->otherData.dirty = true;
						broadcastBlockDestroyed(candidate, *block);
					}
					state.defenseDamage.erase(candidate);
				}
				break;
			}
		}

		for (auto it = state.defenseDamage.begin(); it != state.defenseDamage.end();)
		{
			if (touched.find(it->first) == touched.end()) { it->second -= 4.f * elapsed; }
			if (it->second <= 0.f) { it = state.defenseDamage.erase(it); }
			else { ++it; }
		}
	}

	bool isStandingOnTrap(ServerChunkStorer &chunkStorer, const glm::dvec3 &position)
	{
		const glm::ivec3 base = toBlockPosition(position);
		for (int offsetY = -1; offsetY <= 0; ++offsetY)
		{
			Block *block = chunkStorer.getBlockSafe(base + glm::ivec3(0, offsetY, 0));
			if (block && block->getType() == BlockTypes::woodenSpikeTrap) { return true; }
		}
		return false;
	}

	template<class Container>
	void damageEnemiesOnTraps(Container &container, ServerChunkStorer &chunkStorer,
		std::vector<std::uint64_t> &killed)
	{
		for (auto &entry : container)
		{
			auto &enemy = entry.second;
			if (!isStandingOnTrap(chunkStorer, enemy.getPosition())) { continue; }

			enemy.entity.forces.velocity.x *= 0.35f;
			enemy.entity.forces.velocity.z *= 0.35f;
			if (enemy.entity.life.life <= 14)
			{
				enemy.entity.life.life = 0;
				killed.push_back(entry.first);
			}
			else
			{
				enemy.entity.life.life -= 14;
			}
		}
	}

	void updateSpikeTraps(ServerChunkStorer &chunkStorer, WorldSaver &worldSaver)
	{
		std::vector<std::uint64_t> killed;
		for (auto &chunkEntry : chunkStorer.savedChunks)
		{
			if (!chunkEntry.second || !chunkEntry.second->otherData.withinSimulationDistance) { continue; }
			damageEnemiesOnTraps(chunkEntry.second->entityData.zombies, chunkStorer, killed);
			damageEnemiesOnTraps(chunkEntry.second->entityData.goblins, chunkStorer, killed);
		}
		for (const std::uint64_t entityId : killed) { killEntity(worldSaver, entityId, chunkStorer); }
	}

	void broadcastStatus(const SiegeStatus &status, bool force)
	{
		if (!force && state.hasBroadcastStatus && status == state.lastBroadcastStatus) { return; }
		Packet packet;
		packet.header = headerUpdateSiegeStatus;
		Packet_UpdateSiegeStatus data;
		data.status = status;
		broadCast(packet, &data, sizeof(data), nullptr, true, channelHandleConnections);
		state.lastBroadcastStatus = status;
		state.hasBroadcastStatus = true;
	}

	void broadcastWorldTime(bool advancing)
	{
		Packet packet;
		packet.header = headerUpdateWorldTime;
		Packet_UpdateWorldTime data;
		data.dayPhase = state.worldClock.getDayPhase();
		data.completedCycles = state.worldClock.getCompletedCycles();
		data.advancing = advancing ? 1 : 0;
		broadCast(packet, &data, sizeof(data), nullptr, false, channelHandleConnections);
	}
}

void resetServerSiegeRuntime()
{
	state = ServerSiegeState();
}

bool loadServerSiegeRuntime(const WorldSaver &worldSaver)
{
	std::vector<char> savedData;
	if (sfs::safeLoad(savedData, worldProgressPath(worldSaver).c_str(), false) != sfs::noError)
	{
		return false;
	}

	WorldProgressSnapshot snapshot;
	if (!parseWorldProgressSnapshot(savedData.data(), savedData.size(), snapshot))
	{
		std::cerr << "Warning: ignored invalid world time and siege progress data.\n";
		return false;
	}

	state.worldClock.restore(snapshot.cycleProgressSeconds, snapshot.completedCycles);
	state.director.restoreSchedule(state.worldClock.getVisibleDayNumber(), snapshot.nextSiegeCycle,
		snapshot.completedSieges);
	return true;
}

bool saveServerSiegeRuntime(const WorldSaver &worldSaver)
{
	WorldProgressSnapshot snapshot;
	snapshot.cycleProgressSeconds = state.worldClock.getCycleProgressSeconds();
	snapshot.completedCycles = state.worldClock.getCompletedCycles();
	snapshot.nextSiegeCycle = state.director.getNextSiegeCycle();
	snapshot.completedSieges = state.director.getCompletedSieges();
	const auto data = formatWorldProgressSnapshot(snapshot);
	return !data.empty() && sfs::safeSave(data.data(), data.size(),
		worldProgressPath(worldSaver).c_str(), true) == sfs::noError;
}

void updateServerSiegeRuntime(float deltaTime, ServerChunkStorer &chunkStorer,
	WorldSaver &worldSaver, std::minstd_rand &rng)
{
	pruneMissingSiegeEnemies(chunkStorer);
	const std::vector<std::uint64_t> players = survivalPlayers();
	const bool worldTimeAdvancing = !players.empty();
	const std::uint64_t advancedCycles = state.worldClock.update(deltaTime, worldTimeAdvancing);
	const std::uint64_t previousNextSiegeCycle = state.director.getNextSiegeCycle();
	const SiegePhase previousPhase = state.director.getStatus(
		static_cast<unsigned int>(state.activeEnemyIds.size())).phase;
	const std::uint64_t visibleDay = state.worldClock.getVisibleDayNumber();
	const float dayPhase = state.worldClock.getDayPhase();
	state.director.update(deltaTime, static_cast<unsigned int>(players.size()),
		static_cast<unsigned int>(state.activeEnemyIds.size()),
		visibleDay, state.worldClock.isNight());
	const SiegePhase updatedPhase = state.director.getStatus(
		static_cast<unsigned int>(state.activeEnemyIds.size())).phase;
	if (previousPhase == SiegePhase::Peace && updatedPhase == SiegePhase::Warning &&
		previousNextSiegeCycle != state.director.getNextSiegeCycle())
	{
		state.naturalSiegeActive = true;
		state.naturalSiegeDay = visibleDay;
	}

	const bool reachedFollowingSunrise = state.naturalSiegeActive &&
		visibleDay > state.naturalSiegeDay && dayPhase < 0.25f;
	if (reachedFollowingSunrise)
	{
		removeRemainingNaturalSiegeEnemies(chunkStorer, worldSaver);
	}
	if (advancedCycles != 0 || previousNextSiegeCycle != state.director.getNextSiegeCycle())
	{
		if (!saveServerSiegeRuntime(worldSaver))
		{
			std::cerr << "Warning: could not persist world time and siege schedule.\n";
		}
	}

	state.spawnTimer -= deltaTime;
	if (state.director.getStatus(static_cast<unsigned int>(state.activeEnemyIds.size())).phase == SiegePhase::Wave &&
		state.spawnTimer <= 0.f)
	{
		const unsigned int requested = state.director.takeSpawnRequest(1);
		if (requested != 0 && !spawnOneSiegeEnemy(chunkStorer, worldSaver, rng, players))
		{
			state.director.returnSpawnRequest(requested);
			state.spawnTimer = 1.f;
		}
		else if (requested != 0)
		{
			state.spawnTimer = 0.65f;
		}
	}

	state.defenseTimer -= deltaTime;
	if (state.defenseTimer <= 0.f)
	{
		state.defenseTimer = 0.25f;
		updateBarricadeDamage(0.25f, chunkStorer);
	}

	state.trapTimer -= deltaTime;
	if (state.trapTimer <= 0.f)
	{
		state.trapTimer = 0.5f;
		updateSpikeTraps(chunkStorer, worldSaver);
	}

	pruneMissingSiegeEnemies(chunkStorer);
	state.statusBroadcastTimer -= deltaTime;
	const bool periodicBroadcast = state.statusBroadcastTimer <= 0.f;
	if (periodicBroadcast) { state.statusBroadcastTimer = 1.f; }
	broadcastStatus(state.director.getStatus(
		static_cast<unsigned int>(state.activeEnemyIds.size())), periodicBroadcast);

	state.worldTimeBroadcastTimer -= deltaTime;
	if (state.worldTimeBroadcastTimer <= 0.f)
	{
		state.worldTimeBroadcastTimer = 1.f;
		broadcastWorldTime(worldTimeAdvancing);
	}
}

SiegeStatus getServerSiegeStatus()
{
	return state.director.getStatus(static_cast<unsigned int>(state.activeEnemyIds.size()));
}

bool isServerSiegeWaveActive()
{
	return getServerSiegeStatus().phase == SiegePhase::Wave;
}

bool isServerSiegeEnemy(std::uint64_t entityId)
{
	return state.activeEnemyIds.find(entityId) != state.activeEnemyIds.end();
}

bool forceServerSiegeWarning()
{
	const SiegePhase before = getServerSiegeStatus().phase;
	state.director.forceWarning();
	const bool started = before != getServerSiegeStatus().phase;
	if (started)
	{
		state.naturalSiegeActive = false;
		state.naturalSiegeDay = 0;
	}
	return started;
}
