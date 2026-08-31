#include "multyPlayer/server.h"
#include <glm/vec3.hpp>
#include "chunkSystem.h"
#include "threadStuff.h"
#include <thread>
#include <mutex>
#include <queue>
#include "worldGenerator.h"
#include <unordered_map>
#include <iostream>
#include <atomic>
#include <enet/enet.h>
#include "multyPlayer/packet.h"
#include "multyPlayer/dataIntegrity.h"
#include "multyPlayer/chunkStreamingBudget.h"
#include "multyPlayer/enetServerFunction.h"
#include <platformTools.h>
#include <fstream>
#include <sstream>
#include <structure.h>
#include <biome.h>
#include <unordered_set>
#include <profilerLib.h>
#include "multyPlayer/chunkSaver.h"
#include "multyPlayer/serverChunkStorer.h"
#include <multyPlayer/tick.h>
#include <multyPlayer/splitUpdatesLogic.h>
#include <gameplay/crafting.h>
#include <gameplay/cat.h>
#include <gameplay/pig.h>
#include <gameplay/zombie.h>
#include <gameplay/gameplayRules.h>
#include <gameplay/food.h>
#include <gameplay/serverSiegeRuntime.h>
#include <gameplay/spawnPressure.h>
#include <gameplay/worldDifficulty.h>
#include <native/serverNativeSystems.h>
#include <profiler.h>
#include <magic_enum.hpp>
#include <cmath>
#include <algorithm>
#include <iterator>
#include <vector>

static std::atomic<bool> serverRunning = false;

bool serverStartupStuff(const std::string &path);

bool isServerRunning()
{
	return serverRunning;
}

bool startServer(const std::string &path)
{

	bool expected = 0;
	if (serverRunning.compare_exchange_strong(expected, 1))
	{
		if (!serverStartupStuff(path))
		{
			serverRunning = false;
			return 0;
		}

		return 1;
	}
	else
	{
		return 0;
	}
}


void updateLoadedChunks(
	WorldGenerator &wg,
	StructuresManager &structureManager,
	BiomesManager &biomesManager,
	std::vector<SendBlocksBack> &sendNewBlocksToPlayers,
	WorldSaver &worldSaver, bool generateNewChunks, std::minstd_rand &rng);


struct ServerData
{

	//todo probably move this just locally
	ServerChunkStorer chunkCache = {};
	ENetHost *server = nullptr;
	ServerSettings settings = {};

	float tickTimer = 0;
	int ticksPerSeccond = 0;
	int runsPerSeccond = 0;
	float seccondsTimer = 0;

	float saveEntitiesTimer = 5;
	float passiveSpawnTimer = 4;
	float naturalHostileSpawnTimer = 8;
	std::uint64_t droppedSimulationMilliseconds = 0;

	//this is used as an unique id for chunk packets
	unsigned int chunkPacketId = 0;

}sd;

namespace
{
	std::size_t countAmbientCreatures()
	{
		std::size_t count = 0;
		for (const auto &entry : sd.chunkCache.savedChunks)
		{
			if (!entry.second || !entry.second->otherData.withinSimulationDistance) { continue; }
			count += entry.second->entityData.pigs.size();
			count += entry.second->entityData.cats.size();
			for (const auto &goblin : entry.second->entityData.goblins)
			{
				if (!isServerSiegeEnemy(goblin.first)) { ++count; }
			}
		}
		return count;
	}

	std::size_t countNaturalHostiles()
	{
		std::size_t count = 0;
		for (const auto &entry : sd.chunkCache.savedChunks)
		{
			if (!entry.second || !entry.second->otherData.withinSimulationDistance) { continue; }
			for (const auto &zombie : entry.second->entityData.zombies)
			{
				if (!isServerSiegeEnemy(zombie.first)) { ++count; }
			}
		}
		return count;
	}

	bool unsuitableAmbientGround(BlockType type)
	{
		return type == BlockTypes::water || type == BlockTypes::ice ||
			isAnyLeaves(type) || isAnyWoddenLOG(type) || isDecorativeFurniture(type);
	}

	bool farEnoughFromPlayersForAmbientSpawn(const glm::dvec3 &position,
		const std::unordered_map<std::uint64_t, Client> &clients,
		double minimumDistance)
	{
		const double minimumDistanceSquared = minimumDistance * minimumDistance;
		for (const auto &entry : clients)
		{
			glm::dvec2 delta(position.x - entry.second.playerData.entity.position.x,
				position.z - entry.second.playerData.entity.position.z);
			if (glm::dot(delta, delta) < minimumDistanceSquared) { return false; }
		}
		return true;
	}
	bool findAmbientSpawn(const glm::dvec3 &playerPosition,
		std::minstd_rand &rng, glm::dvec3 &spawnPosition,
		float minimumSpawnDistance, float maximumSpawnDistance,
		double minimumPlayerDistance)
	{
		constexpr float pi = 3.14159265358979323846f;
		for (int attempt = 0; attempt < 16; ++attempt)
		{
			const float angle = getRandomNumberFloat(rng, 0.f, pi * 2.f);
			const float distance = getRandomNumberFloat(rng,
				minimumSpawnDistance, maximumSpawnDistance);
			const int worldX = static_cast<int>(std::floor(playerPosition.x + std::cos(angle) * distance));
			const int worldZ = static_cast<int>(std::floor(playerPosition.z + std::sin(angle) * distance));

			auto *chunk = sd.chunkCache.getChunkOrGetNull(divideChunk(worldX), divideChunk(worldZ));
			if (!chunk || !chunk->otherData.withinSimulationDistance) { continue; }

			const int localX = modBlockToChunk(worldX);
			const int localZ = modBlockToChunk(worldZ);
			for (int y = CHUNK_HEIGHT - 3; y >= 2; --y)
			{
				auto *ground = chunk->chunk.safeGet(localX, y, localZ);
				auto *feet = chunk->chunk.safeGet(localX, y + 1, localZ);
				auto *head = chunk->chunk.safeGet(localX, y + 2, localZ);
				if (!ground || !feet || !head) { continue; }
				if (!ground->isColidable() || unsuitableAmbientGround(ground->getType())) { continue; }
				if (!feet->air() || !head->air()) { continue; }

				spawnPosition = glm::dvec3(worldX, y + 0.51, worldZ);
				if (farEnoughFromPlayersForAmbientSpawn(spawnPosition,
					getAllClientsReff(), minimumPlayerDistance)) { return true; }
				break;
			}
		}
		return false;
	}
	std::vector<std::uint64_t> livingSurvivalPlayers()
	{
		std::vector<std::uint64_t> result;
		for (const auto &entry : getAllClientsReff())
		{
			if (!entry.second.playerData.killed &&
				entry.second.playerData.otherPlayerSettings.gameMode ==
				OtherPlayerSettings::SURVIVAL)
			{
				result.push_back(entry.first);
			}
		}
		return result;
	}

	void updateAmbientEcologySpawning(float deltaTime, WorldSaver &worldSaver,
		std::minstd_rand &rng, const NaturalSpawnPressure &pressure)
	{
		sd.passiveSpawnTimer -= deltaTime;
		if (sd.passiveSpawnTimer > 0.f) { return; }
		sd.passiveSpawnTimer = getRandomNumberFloat(rng,
			pressure.passiveIntervalMin, pressure.passiveIntervalMax);

		auto &clients = getAllClientsReff();
		if (clients.empty() || pressure.passiveCap == 0 ||
			countAmbientCreatures() >= pressure.passiveCap) { return; }

		auto selectedClient = clients.begin();
		std::advance(selectedClient, getRandomNumber(rng, 0, static_cast<int>(clients.size()) - 1));
		glm::dvec3 spawnPosition;
		if (!findAmbientSpawn(selectedClient->second.playerData.entity.position,
			rng, spawnPosition, 18.f, 42.f, 14.0))
		{
			return;
		}
		const int creatureRoll = getRandomNumber(rng, 0, 99);
		if (creatureRoll < 50)
		{
			Pig pig;
			pig.position = spawnPosition;
			pig.lastPosition = spawnPosition;
			spawnPig(sd.chunkCache, pig, worldSaver, rng);
		}
		else if (creatureRoll < 75)
		{
			Cat cat;
			cat.position = spawnPosition;
			cat.lastPosition = spawnPosition;
			spawnCat(sd.chunkCache, cat, worldSaver, rng);
		}
		else
		{
			Goblin goblin;
			goblin.position = spawnPosition;
			goblin.lastPosition = spawnPosition;
			spawnGoblin(sd.chunkCache, goblin, worldSaver, rng, nullptr, true);
		}
	}

	void updateNightHostileSpawning(float deltaTime, WorldSaver &worldSaver,
		std::minstd_rand &rng, const NaturalSpawnPressure &pressure,
		const std::vector<std::uint64_t> &survivalPlayers)
	{
		if (!pressure.hostileSpawningEnabled || survivalPlayers.empty()) { return; }
		sd.naturalHostileSpawnTimer -= deltaTime;
		if (sd.naturalHostileSpawnTimer > 0.f) { return; }
		sd.naturalHostileSpawnTimer = getRandomNumberFloat(rng,
			pressure.hostileIntervalMin, pressure.hostileIntervalMax);
		if (countNaturalHostiles() >= pressure.hostileCap) { return; }

		const std::uint64_t targetId = survivalPlayers[static_cast<std::size_t>(
			getRandomNumber(rng, 0, static_cast<int>(survivalPlayers.size()) - 1))];
		auto foundTarget = getAllClientsReff().find(targetId);
		if (foundTarget == getAllClientsReff().end()) { return; }

		glm::dvec3 spawnPosition;
		if (!findAmbientSpawn(foundTarget->second.playerData.entity.position,
			rng, spawnPosition, 22.f, 46.f, 18.0))
		{
			return;
		}

		Zombie zombie;
		zombie.position = spawnPosition;
		zombie.lastPosition = spawnPosition;
		const std::uint64_t newId = getEntityIdAndIncrement(worldSaver,
			EntityType::zombies);
		if (!spawnZombie(sd.chunkCache, zombie, newId)) { return; }
		auto *chunk = sd.chunkCache.getChunkOrGetNull(
			divideChunk(spawnPosition.x), divideChunk(spawnPosition.z));
		if (!chunk) { return; }
		auto foundZombie = chunk->entityData.zombies.find(newId);
		if (foundZombie != chunk->entityData.zombies.end())
		{
			foundZombie->second.forceTarget(targetId);
		}
	}

	bool findSafeSpawnColumn(int worldX, int worldZ, glm::dvec3 &result)
	{
		auto *chunk = sd.chunkCache.getChunkOrGetNull(divideChunk(worldX), divideChunk(worldZ));
		if (!chunk) { return false; }

		const int localX = modBlockToChunk(worldX);
		const int localZ = modBlockToChunk(worldZ);
		for (int feetY = CHUNK_HEIGHT - 2; feetY >= 1; --feetY)
		{
			Block *ground = chunk->chunk.safeGet(localX, feetY - 1, localZ);
			Block *feet = chunk->chunk.safeGet(localX, feetY, localZ);
			Block *head = chunk->chunk.safeGet(localX, feetY + 1, localZ);
			if (!ground || !feet || !head) { continue; }
			if (!ground->isColidable() || unsuitableAmbientGround(ground->getType())) { continue; }
			if (!feet->air() || !head->air()) { continue; }
			result = glm::dvec3(worldX, feetY, worldZ);
			return true;
		}
		return false;
	}
}

int outTicksPerSeccond = 0;

int getServerTicksPerSeccond()
{
	return outTicksPerSeccond;
}

ServerChunkStorer &getServerChunkStorer()
{
	return sd.chunkCache;
}

bool tryResolveSafeServerSpawn(WorldSaver &worldSaver, glm::dvec3 &safePosition)
{
	const glm::ivec3 preferred = worldSaver.spawnPosition;
	for (int radius = 0; radius <= 4; ++radius)
	{
		for (int dz = -radius; dz <= radius; ++dz)
		{
			for (int dx = -radius; dx <= radius; ++dx)
			{
				if (radius != 0 && std::max(std::abs(dx), std::abs(dz)) != radius) { continue; }
				if (findSafeSpawnColumn(preferred.x + dx, preferred.z + dz, safePosition))
				{
					worldSaver.spawnPosition = glm::ivec3(safePosition);
					return true;
				}
			}
		}
	}
	return false;
}

glm::dvec3 resolveSafeServerSpawn(WorldSaver &worldSaver)
{
	glm::dvec3 safePosition = glm::dvec3(worldSaver.spawnPosition);
	tryResolveSafeServerSpawn(worldSaver, safePosition);
	return safePosition;
}

void clearSD(WorldSaver &worldSaver)
{
	//todo saveEntityId stuff
	//worldSaver.saveEntityId(getCurrentEntityId());
	sd.chunkCache.saveAllChunks(worldSaver);
	sd.chunkCache.cleanup();
	closeThreadPool();
}

int getChunkCapacity()
{
	return sd.chunkCache.savedChunks.size();
}

void closeServer()
{
	//todo cleanup stuff
	if (serverRunning)
	{

		closeEnetListener();


		//close loop
		serverRunning = false;

		//then signal the barier from the task waiting to unlock the mutex

		//then wait for the server to close
		//serverThread.join();

		enet_host_destroy(sd.server);

		//todo clear othher stuff
		sd = {};
	}

	//serverSettingsMutex.unlock();
}


//Note: it is a problem that the block validation and the item validation are on sepparate threads.
bool computeRevisionStuff(Client &client, bool allowed, 
	const EventId &eventId, std::uint64_t *oldid, std::uint64_t *newid)
{

	permaAssertComment((oldid == 0 && newid == 0) || (oldid != 0 && newid != 0),
		"both ids should be supplied or none");


	bool noNeedToNotifyUndo = false;

	if (client.revisionNumber > eventId.revision)
	{
		//if the revision number is increased it means that we already undoed all those moves
		allowed = false;
		noNeedToNotifyUndo = true;
		//std::cout << "Server revision number ignore: " << client->revisionNumber << " "
		//	<< i.t.eventId.revision << "\n";
	}


	//validate event
	if(allowed)
	{
		if (oldid && newid)
		{
			Packet packet;
			packet.header = headerValidateEventAndChangeID;

			Packet_ValidateEventAndChangeId packetData;
			packetData.eventId = eventId;
			packetData.oldId = *oldid;
			packetData.newId = *newid;

			sendPacket(client.peer, packet,
				(char *)&packetData, sizeof(Packet_ValidateEventAndChangeId),
				true, channelChunksAndBlocks);
		}
		else
		{
			Packet packet;
			packet.header = headerValidateEvent;

			Packet_ValidateEvent packetData;
			packetData.eventId = eventId;

			sendPacket(client.peer, packet, (char *)&packetData,
				sizeof(Packet_ValidateEvent), true, channelChunksAndBlocks);
		}
		
	}
	else if (!noNeedToNotifyUndo)
	{
		Packet packet;
		//packet.cid = i.cid;
		packet.header = headerInValidateEvent;

		Packet_InValidateEvent packetData;
		packetData.eventId = eventId;

		client.revisionNumber++;

		sendPacket(client.peer, packet, (char *)&packetData,
			sizeof(Packet_ValidateEvent), true, channelChunksAndBlocks);
	}

	return allowed;
}

bool serverStartupStuff(const std::string &path)
{
	//reset data
	sd = ServerData{};
	resetServerSiegeRuntime();
	resetServerWorldDifficultySettings();
	mie::native::resetServerNativeSystems();


	//start enet server
	ENetAddress adress;
	adress.host = ENET_HOST_ANY;
	adress.port = 7771;
	ENetEvent event;

	//first param adress, players limit, channels, bandwith limit
	sd.server = enet_host_create(&adress, 32, SERVER_CHANNELS, 0, 0);


	if (!sd.server)
	{
		//todo some king of error reporting to the player
		return 0;
	}

	if (!startEnetListener(sd.server, path))
	{
		enet_host_destroy(sd.server);
		sd.server = 0;
		return 0;
	}

	return true;
}


void updateOtherPlayerSettings(Client &client)
{
	Packet_UpdateOwnOtherPlayerSettings packet;
	packet.otherPlayerSettings = client.playerData.otherPlayerSettings;

	sendPacket(client.peer, headerUpdateOwnOtherPlayerSettings,
		&packet, sizeof(packet), true, channelChunksAndBlocks);
}

void changePlayerGameMode(std::uint64_t cid, unsigned char gameMode)
{

	if (auto client = getClientNotLocked(cid))
	{
		if (client->playerData.otherPlayerSettings.gameMode != gameMode)
		{
			client->playerData.otherPlayerSettings.gameMode = gameMode;
			updateOtherPlayerSettings(*client);
		}
	}
}


ServerSettings getServerSettingsCopy()
{
	return sd.settings;
}

ServerSettings &getServerSettingsReff()
{
	return sd.settings;
}

unsigned int getRandomTickSpeed()
{
	return sd.settings.randomTickSpeed;
}

void setServerSettings(ServerSettings settings)
{
	for (auto &s : sd.settings.perClientSettings)
	{
		auto it = settings.perClientSettings.find(s.first);
		if (it != settings.perClientSettings.end())
		{
			s.second = it->second;
		}
	}
}

void genericBroadcastEntityDeleteFromServerToPlayer(std::uint64_t eid, bool reliable, 
	std::unordered_map<std::uint64_t, Client *> &allClients, 
	glm::ivec2 lastChunkClientsGotUpdates)
{
	Packet packet;
	packet.header = headerRemoveEntity;

	Packet_RemoveEntity data;
	data.EID = eid;

	broadCast(packet, &data, sizeof(data),
		nullptr, reliable, channelEntityPositions);
}


void genericBroadcastEntityKillFromServerToPlayer(std::uint64_t eid, bool reliable, ENetPeer *peerToIgnore)
{
	Packet packet;
	packet.header = headerKillEntity;

	Packet_KillEntity data;
	data.EID = eid;

	broadCast(packet, &data, sizeof(data),
		peerToIgnore, reliable, channelEntityPositions);
}

void serverWorkerUpdate(
	WorldGenerator &wg,
	StructuresManager &structuresManager,
	BiomesManager &biomesManager,
	WorldSaver &worldSaver,
	std::vector<ServerTask> &serverTask,
	float deltaTime, Profiler &serverProfiler
	)
{

#pragma region timers stuff
	auto currentTimer = getTimer();
	if (!std::isfinite(deltaTime) || deltaTime < 0.f) { deltaTime = 0.f; }
	const float boundedDeltaTime = std::min(deltaTime, 0.25f);
	sd.tickTimer += boundedDeltaTime;
	sd.seccondsTimer += boundedDeltaTime;
	sd.saveEntitiesTimer -= boundedDeltaTime;
#pragma endregion

	auto &settings = sd.settings;

	static std::minstd_rand rng(std::random_device{}());
	// Streaming performs integer block/chunk conversion before the gameplay tick.
	// Repair persisted or network-corrupted state first so NaN/Inf never reaches it.
	for (auto &clientEntry : getAllClientsReff())
	{
		auto &entity = clientEntry.second.playerData.entity;
		entity.chunkDistance = std::clamp(entity.chunkDistance, 2, 24);
		if (mie::dataIntegrity::isFiniteWorldPosition(entity.position) &&
			mie::dataIntegrity::isDroppedItemMotionStateValid(entity.forces))
		{
			continue;
		}

		glm::dvec3 safePosition = glm::dvec3(worldSaver.spawnPosition);
		const bool resolved = tryResolveSafeServerSpawn(worldSaver, safePosition);
		entity.position = safePosition;
		entity.lastPosition = safePosition;
		entity.forces = {};
		clientEntry.second.needsSafeSpawnPlacement = !resolved;
	}


#pragma region send chunks to players

	serverProfiler.startSubProfile("Send Chunks To players");

	std::vector<SendBlocksBack> sendNewBlocksToPlayers;
	bool generateNewChunks = true;
	if (sd.seccondsTimer * targetTicksPerSeccond >= sd.runsPerSeccond)
	{
		generateNewChunks = false; // the server can potentially lag a little, so we stop sending chunks
	}

	if (sd.ticksPerSeccond < 5)
	{
		//make sure we still generate at least a few chunks even though the server is lagging
		generateNewChunks = true;
	}

	updateLoadedChunks(wg, structuresManager, biomesManager, sendNewBlocksToPlayers,
		worldSaver, generateNewChunks, rng);

	serverProfiler.endSubProfile("Send Chunks To players");


#pragma endregion


#pragma region unload chunks
	serverProfiler.startSubProfile("Unload chunks");
	sd.chunkCache.unloadChunksThatNeedUnloading(worldSaver, 2);
	serverProfiler.endSubProfile("Unload chunks");
#pragma endregion



	//here used to be the tasks


	//todo check if there are too many loaded chunks and unload them before processing
	//generate chunk

#pragma region gameplay tick


	constexpr float fixedTickDeltaTime = 1.f / targetTicksPerSeccond;
	constexpr int fixedTickDeltaTimeMs = 1000 / targetTicksPerSeccond;
	int catchUpTicks = 0;
	while (sd.tickTimer >= fixedTickDeltaTime && catchUpTicks < 4)
	{
		++catchUpTicks;

	#pragma region set players in their chunks
		for (auto &c : sd.chunkCache.savedChunks)
		{
			c.second->entityData.players.clear();
		}

		//todo move in tick probably
		//set players in their chunks, set players in chunks

		for (auto &client : getAllClientsReff())
		{
			if (client.second.needsSafeSpawnPlacement)
			{
				glm::dvec3 safePosition;
				if (tryResolveSafeServerSpawn(worldSaver, safePosition))
				{
					client.second.playerData.entity.position = safePosition;
					client.second.playerData.entity.lastPosition = safePosition;
					client.second.playerData.entity.forces = {};
					client.second.needsSafeSpawnPlacement = false;

					Packet packet;
					packet.cid = client.first;
					packet.header = headerRespawnPlayer;
					Packet_RespawnPlayer packetData;
					packetData.pos = safePosition;
					sendPacket(client.second.peer, packet, reinterpret_cast<const char *>(&packetData),
						sizeof(packetData), true, channelChunksAndBlocks);
				}
			}

			if (!mie::dataIntegrity::isFiniteWorldPosition(client.second.playerData.entity.position) ||
				!mie::dataIntegrity::isDroppedItemMotionStateValid(
					client.second.playerData.entity.forces))
			{
				glm::dvec3 safePosition;
				if (tryResolveSafeServerSpawn(worldSaver, safePosition))
				{
					client.second.playerData.entity.position = safePosition;
					client.second.playerData.entity.lastPosition = safePosition;
					client.second.playerData.entity.forces = {};
				}
				else
				{
					client.second.needsSafeSpawnPlacement = true;
					continue;
				}
			}

			auto cPos = determineChunkThatIsEntityIn(client.second.playerData.entity.position);

			auto chunk = sd.chunkCache.getChunkOrGetNull(cPos.x, cPos.y);

			if (!chunk)
			{
				client.second.needsSafeSpawnPlacement = true;
				continue;
			}

			chunk->entityData.players[client.first] = &client.second.playerData;
			sd.chunkCache.entityChunkPositions[client.first] = cPos;

		}

		updateServerSiegeRuntime(fixedTickDeltaTime, sd.chunkCache, worldSaver, rng);
		mie::native::updateServerNativeSystems(fixedTickDeltaTime);
		const auto survivalPlayers = livingSurvivalPlayers();
		const SiegeStatus siegeStatus = getServerSiegeStatus();
		const NaturalSpawnPressure spawnPressure = getNaturalSpawnPressure(
			getServerWorldDayPhase(), getServerVisibleWorldDay(),
			getAllClientsReff().size(), survivalPlayers.size(),
			getServerWorldDifficultySettings(), siegeStatus.phase != SiegePhase::Peace);
		if (!isServerSiegeWaveActive())
		{
			updateAmbientEcologySpawning(fixedTickDeltaTime, worldSaver, rng,
				spawnPressure);
		}
		updateNightHostileSpawning(fixedTickDeltaTime, worldSaver, rng,
			spawnPressure, survivalPlayers);

	#pragma endregion


		//ALL CHUNKS THAT PLAYERS ARE IN SHOULD BE LOADED!!!!


		//for (auto &c : sd.chunkCache.savedChunks)
		//{
		//	c.second->entityData.players.clear();
		//}
		//
		//for (auto &client : getAllClients())
		//{
		//
		//	auto cPos = determineChunkThatIsEntityIn(client.second.playerData.entity.position);
		//	
		//	auto chunk = sd.chunkCache.getChunkOrGetNull(cPos.x, cPos.y);
		//
		//	permaAssertComment(chunk, "Error, A chunk that a player is in unloaded...");
		//
		//	chunk->entityData.players.insert({client.first, &client.second.playerData});
		//
		//}

	#pragma region replace spawn position
		resolveSafeServerSpawn(worldSaver);
	#pragma endregion


		sd.tickTimer -= fixedTickDeltaTime;
		// Timestamp each recovered tick at its actual position in the backlog.
		// Reusing currentTimer for every catch-up step produces multiple entity
		// snapshots with the same timestamp and breaks client interpolation.
		const std::uint64_t pendingSimulationMilliseconds = static_cast<std::uint64_t>(
			std::max(0.f, sd.tickTimer) * 1000.f);
		const std::uint64_t tickCurrentTimer = currentTimer > pendingSimulationMilliseconds
			? currentTimer - pendingSimulationMilliseconds : 0;

		sd.ticksPerSeccond++;

		if(settings.perClientSettings.size())
		{


			if (settings.perClientSettings.begin()->second.resendInventory)
			{
				settings.perClientSettings.begin()->second.resendInventory = false;
				auto &c = getAllClientsReff();

				sendPlayerInventoryAndIncrementRevision(c.begin()->second);
			}

			if (settings.perClientSettings.begin()->second.damage)
			{
				settings.perClientSettings.begin()->second.damage = false;
				auto &c = getAllClientsReff();

				c.begin()->second.playerData.applyDamageOrLife(-10);
			}

			if (settings.perClientSettings.begin()->second.heal)
			{
				settings.perClientSettings.begin()->second.heal = false;
				auto &c = getAllClientsReff();

				c.begin()->second.playerData.applyDamageOrLife(10);
			}

			if (settings.perClientSettings.begin()->second.generateStructure)
			{
				settings.perClientSettings.begin()->second.generateStructure = false;
				auto &c = getAllClientsReff();

				glm::ivec3 pos = c.begin()->second.playerData.getPosition();
				pos.y -= 21;
					
				StructureToGenerate s;
				s.type = Structure_MinesDungeon;
				s.randomNumber1 = getRandomNumberFloat(rng, 0, 1);
				s.randomNumber2 = getRandomNumberFloat(rng, 0, 1);
				s.randomNumber3 = getRandomNumberFloat(rng, 0, 1);
				s.randomNumber4 = getRandomNumberFloat(rng, 0, 1);
				s.pos = pos;
				s.replaceBlocks = true;

				std::unordered_map<glm::ivec2, SavedChunk *, Ivec2Hash> newCreatedOrLoadedChunks;
				std::vector<glm::ivec3> controlBlocks;
				sd.chunkCache.generateStructure(s, structuresManager, newCreatedOrLoadedChunks,
					sendNewBlocksToPlayers, &controlBlocks);

			}


			//TODO chunks shouldn't be nullptrs so why check them?
			//	// so maybe just perma assert comment at the beginning

			//if (settings.perClientSettings.begin()->second.killApig)
			//{
			//	settings.perClientSettings.begin()->second.killApig = false;
			//
			//	
			//
			//	for (auto &c : sd.chunkCache.savedChunks)
			//	{
			//		if (c.second && c.second->entityData.pigs.size())
			//		{
			//			killEntity(worldSaver, c.second->entityData.pigs.begin()->first);
			//			break;
			//		}
			//	}
			//
			//}
		}



		//todo error and warning logs for server.


		//todo get all clients should probably dissapear.
		auto &clients = getAllClientsReff();

		for (auto &c : clients)
		{
			c.second.playerData.inventory.sanitize();
		}

		splitUpdatesLogic(fixedTickDeltaTime, fixedTickDeltaTimeMs,
			tickCurrentTimer, sd.chunkCache, rng(), clients, worldSaver, serverTask,
			serverProfiler);
	}
	if (catchUpTicks == 4 && sd.tickTimer >= fixedTickDeltaTime)
	{
		const float droppedSeconds = sd.tickTimer - std::fmod(sd.tickTimer, fixedTickDeltaTime);
		sd.droppedSimulationMilliseconds += static_cast<std::uint64_t>(
			std::max(0.f, droppedSeconds) * 1000.f);
		sd.tickTimer = std::fmod(sd.tickTimer, fixedTickDeltaTime);
	}

	//std::cout << deltaTime << " <- dt / 1/dt-> " << (1.f / (deltaTime)) << "\n";

	//std::cout << seccondsTimer << '\n';

	sd.runsPerSeccond++;

	if (sd.seccondsTimer >= 1)
	{
		sd.seccondsTimer -= 1;
		sd.seccondsTimer = std::min(sd.seccondsTimer, 1.f);
		//std::cout << "Server ticks per seccond: " << sd.ticksPerSeccond << "\n";
		//std::cout << "Server runs per seccond: " << sd.runsPerSeccond << "\n";
		outTicksPerSeccond = sd.ticksPerSeccond;
		sd.ticksPerSeccond = 0;
		sd.runsPerSeccond = 0;
	}

#pragma endregion

	//this are blocks created by new chunks so everyone needs them
	if (!sendNewBlocksToPlayers.empty())
	{
		Packet_PlaceBlocks *newBlocks = new Packet_PlaceBlocks[sendNewBlocksToPlayers.size()];

		Packet packet;
		packet.cid = 0;
		packet.header = headerPlaceBlocks;

		int i = 0;
		for (auto &b : sendNewBlocksToPlayers)
		{
			//todo an option to send multiple blocks per place block
			//std::cout << "Sending block...";

			//Packet packet;
			//packet.cid = 0;
			//packet.header = headerPlaceBlock;
			//
			//Packet_PlaceBlock packetData;
			//packetData.blockPos = b.pos;
			//packetData.blockType = b.block;
			//
			//broadCast(packet, &packetData, sizeof(Packet_PlaceBlock), nullptr, true, channelChunksAndBlocks);

			newBlocks[i].blockPos = b.pos;
			newBlocks[i].blockInfo = b.blockInfo;

			i++;
		}

		broadCast(packet, newBlocks,
			sizeof(Packet_PlaceBlocks) * sendNewBlocksToPlayers.size(),
			nullptr, true, channelChunksAndBlocks);


		delete[] newBlocks;
	}

#pragma region save stuff
	//save one chunk on disk
	serverProfiler.startSubProfile("Save chunk on disk");
	sd.chunkCache.saveNextChunk(worldSaver);
	serverProfiler.endSubProfile("Save chunk on disk");

	// Chunks already scheduled for unload are saved by the unload path. Do not
	// enqueue the same sidecar twice on the periodic pass, which is especially
	// expensive on HDDs.
	if (sd.saveEntitiesTimer <= 0)
	{
		sd.saveEntitiesTimer = 5;

		for (auto &c : sd.chunkCache.savedChunks)
		{
			if (c.second && !c.second->otherData.shouldUnload)
			{
				c.second->otherData.dirtyEntity = true;
			}
		}

	}
#pragma endregion

}



std::uint64_t getTimer()
{
	static const auto start_time = std::chrono::steady_clock::now();
	auto now = std::chrono::steady_clock::now();
	auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
	return millis;
}


void addCidToServerSettings(std::uint64_t cid)
{
	sd.settings.perClientSettings.insert({cid, {}});
}

void removeCidFromServerSettings(std::uint64_t cid)
{
	sd.settings.perClientSettings.erase(cid);
}


void onPacketDestroyForChunkSending(ENetPacket *packet)
{
	unsigned int userData = static_cast<unsigned int>(reinterpret_cast<std::uintptr_t>(packet->userData));
	
	Packet p = {};
	size_t dataSize = 0;
	parsePacket(*packet, p, dataSize);

	auto cid = p.cid;


	auto &clients = getAllClientsReff();

	auto found = clients.find(cid);
	if (found != clients.end())
	{
		auto rez = found->second.chunksPacketPendingConfirmation.erase(userData);
		int a = 0;
	}

	// Custom logic for when the packet is destroyed
	//std::cout << "Packet of size " << packet->dataLength << " was destroyed (acknowledged or dropped)." << std::endl;
}


//adds loaded chunks.
void updateLoadedChunks(
	WorldGenerator &wg,
	StructuresManager &structureManager,
	BiomesManager &biomesManager,
	std::vector<SendBlocksBack> &sendNewBlocksToPlayers,
	WorldSaver &worldSaver, bool generateNewChunks, std::minstd_rand &rng)
{


	constexpr const int MAX_GENERATE = 1;
	constexpr const int MAX_LOAD = 5;

	for (auto &c : sd.chunkCache.savedChunks)
	{
		c.second->otherData.shouldUnload = true;
		c.second->otherData.withinSimulationDistance = false;
	}

	auto &clients = getAllClientsReff();


	//todo a better way to prioritize ordering and stuff
	std::vector<glm::ivec2> positions;
	positions.reserve(200);

	std::vector<uint64_t> cids;
	cids.reserve(clients.size());

	for (auto &cl : clients)
	{
		cids.push_back(cl.first);
	}

	std::shuffle(cids.begin(), cids.end(), rng);


	int geenratedThisFrame = 0;
	int loadedThisFrame = 0;
	for (auto cid : cids)
	{

		auto &client = clients[cid];
		//if (c.second.playerData.killed) { continue; }


		glm::ivec2 pos(divideChunk(client.playerData.entity.position.x),
			divideChunk(client.playerData.entity.position.z));
		
		auto playerBlockPos = from3DPointToBlock(client.playerData.entity.position);

		int distance = (client.playerData.entity.chunkDistance/2) + 1;

		auto clientCid = cid;

		//drop chunks that are too far
		{
			for (auto it = client.loadedChunks.begin(); it != client.loadedChunks.end();)
			{

				if (!isChunkInRadius({playerBlockPos.x, playerBlockPos.z}, *it, client.playerData.entity.chunkDistance))
				{
					it = client.loadedChunks.erase(it);
				}
				else
				{
					++it;
				}
			}
		}
		positions.clear();

		for (int i = -distance; i <= distance; i++)
			for (int j = -distance; j <= distance; j++)
			{
				glm::vec2 vect(i, j);
				auto chunkPos = pos + glm::ivec2(i, j);

				if (isChunkInRadius({playerBlockPos.x, playerBlockPos.z}, 
					chunkPos, client.playerData.entity.chunkDistance))
				{
					//if (client.loadedChunks.find(chunkPos) ==
					//	client.loadedChunks.end())
					{
						positions.push_back({chunkPos});
					}
				}
			}
		
		//make sure we send the right chunks if the player is right at the border corner between 4 chunks
		auto posAugmentedForSort = glm::ivec2(divideChunk(playerBlockPos.x-1), divideChunk(playerBlockPos.z-1));
		std::sort(positions.begin(), positions.end(),
			[&](auto &a, auto &b)
		{
			glm::vec2 diff1 = a - posAugmentedForSort;
			float distance1 = glm::dot(diff1, diff1);

			glm::vec2 diff2 = b - posAugmentedForSort;
			float distance2 = glm::dot(diff2, diff2);

			return distance1 < distance2;
		});

		bool generatedChunkPlayerIsIn = 0;
		for (auto chunkPos : positions)
		{
			SavedChunk *c = 0;

			bool generateMoreChunks = true;
			if (geenratedThisFrame >= MAX_GENERATE)generateMoreChunks = false;
			if (loadedThisFrame >= MAX_LOAD)generateMoreChunks = false;

			bool canSendMoreChunks = true;


			if ((generateNewChunks && (generateMoreChunks))
				
				//always generate the chunk that the player is in
				|| (chunkPos == pos))
			{

				if (chunkPos == pos) { generatedChunkPlayerIsIn = true; }
				bool generated = 0;
				bool loaded = 0;

				//generate new chunks! (or load them)
				c = sd.chunkCache.getOrCreateChunk(chunkPos.x, chunkPos.y,
					wg, structureManager, biomesManager, sendNewBlocksToPlayers, 
					worldSaver, &generated, &loaded
				);

				if (generated)
				{
					geenratedThisFrame++;
				}
				
				if(loaded)
				{
					loadedThisFrame++;
				}
			}
		

			//number of chunks that are being sent rn
			//stop sending more chunks than the pending number
			const std::size_t currentPendingChunks =
				client.chunksPacketPendingConfirmation.size();
			if (!mie::network::canQueueAnotherChunkPacket(currentPendingChunks))
			{
				canSendMoreChunks = false;
			}


			//always generate the chunk player is in,
			{

				if (!c)
				{
					c = sd.chunkCache.getChunkOrGetNull(chunkPos.x, chunkPos.y);
				}

				if (c)
				{
					c->otherData.shouldUnload = false;

					if (isChunkInRadius({playerBlockPos.x, playerBlockPos.z},
						chunkPos, getServerSettingsReff().simulationDistanceRadius*2))
					{
						c->otherData.withinSimulationDistance = true;
					}


					//send chunk to player
				#pragma region send chunk to player

					if (client.loadedChunks.find(chunkPos) ==
						client.loadedChunks.end() && (canSendMoreChunks || chunkPos == pos))
					{

						client.loadedChunks.insert(chunkPos);

						Packet packet;
						packet.header = headerRecieveChunk;
						packet.cid = clientCid;

						//if you have modified Packet_RecieveChunk make sure you didn't break this!
						static_assert(sizeof(Packet_RecieveChunk) == sizeof(ChunkData));

						{
							//TODO merge this 2 packets into one!

							client.chunksPacketPendingConfirmation.insert(sd.chunkPacketId);

							sendPacketAndCompress(client.peer, packet, (char *)(&c->chunk),
								sizeof(Packet_RecieveChunk), true, channelChunksAndBlocks,
								onPacketDestroyForChunkSending, sd.chunkPacketId++);


							std::vector<unsigned char> blockData;
							c->blockData.formatBlockData(blockData, c->chunk.x, c->chunk.z);

							if (blockData.size())
							{
								Packet packet;
								packet.header = headerRecieveEntireBlockDataForChunk;

								if (blockData.size() > 100)
								{
									sendPacketAndCompress(client.peer, packet, (char *)blockData.data(),
										blockData.size(), true, channelChunksAndBlocks);
								}
								else
								{
									sendPacket(client.peer, packet, (char *)blockData.data(),
										blockData.size(), true, channelChunksAndBlocks);
								};

							}
						}
					}
				#pragma endregion




				}

			};

			

		};


	}


};
	


enum TokenType : int
{
	None,
	Identifier,
	Number,
	Symbol
};

struct TokenCommand
{
	int type = 0;
	std::string value = "";
	double number = 0;
	
};

bool isValidNumber(const std::string &str, double &outValue)
{
	char *end = nullptr;
	outValue = std::strtod(str.c_str(), &end);
	return end != str.c_str() && *end == '\0'; // Ensure full string was parsed
}

std::vector<TokenCommand> parse(const char *input, std::string &errOut)
{
	std::vector<TokenCommand> tokens;
	std::istringstream stream(input);
	std::string token;
	errOut = "";

	while (stream >> token)
	{
		size_t i = 0;
	
		while (i < token.size())
		{
			// Handle symbols
			if (std::ispunct(token[i]) && token[i] != '_')
			{
				tokens.push_back({TokenType::Symbol, std::string(1, token[i])});
				++i;
			}
			// Handle numbers
			else if (std::isdigit(token[i]) || (token[i] == '.' && i + 1 < token.size() && std::isdigit(token[i + 1])))
			{
				size_t start = i;
				while (i < token.size() && (std::isdigit(token[i]) || token[i] == '.')) ++i;
				std::string numStr = token.substr(start, i - start);
	
				double numValue = 0.0;
				if (isValidNumber(numStr, numValue))
				{
					tokens.push_back({TokenType::Number, numStr, numValue});
				}
				else
				{
					errOut = "Error parsing number";
					return {};
					tokens.push_back({TokenType::Number, numStr, 0}); // Invalid number
				}
			}
			// Handle identifiers
			else if (std::isalpha(token[i]) || token[i] == '_')
			{
				size_t start = i;
				while (i < token.size() && (std::isalnum(token[i]) || token[i] == '_')) ++i;
				tokens.push_back({TokenType::Identifier, token.substr(start, i - start)});
			}
			// Skip unknown characters (e.g., spaces handled by `stream >> token`)
			else
			{
				++i;
			}
		}
	}

	return tokens;
}


std::string executeServerCommand(std::uint64_t cid, const char *command)
{
	Client *client = nullptr;
	int commandPermisionLevel = 0;

	if (cid)
	{
		client = getClientSafe(cid);
		if (!client) { return "Error, client not existing, " + std::to_string(cid); }
		commandPermisionLevel = client->playerData.otherPlayerSettings.commandPermisionLevel;
	}
	else
	{
		//command from the server console
		commandPermisionLevel = 3;
	}

	std::string err;

	std::vector<TokenCommand> tokens = parse(command, err);
	if (err != "") { return err; }

	//for (const auto &token : tokens)
	//{
	//	std::string type;
	//	switch (token.type)
	//	{
	//	case TokenType::Identifier: type = "Identifier"; break;
	//	case TokenType::Number:     type = "Number"; break;
	//	case TokenType::Symbol:     type = "Symbol"; break;
	//	}
	//
	//	std::cout << type << ": " << token.value;
	//	if (token.type == TokenType::Number)
	//	{
	//		std::cout << " (double: " << token.number << ")";
	//	}
	//	std::cout << '\n';
	//}

	//parse commands
	{
		int position = 0;

		auto isEof = [&]()
		{
			return position >= tokens.size();
		};

		auto consumeStringToken = [&](std::string s)
		{
			if (isEof()) { return false; }

			if (tokens[position].type == Identifier &&
				tokens[position].value == s)
			{
				position++;
				return true;
			}

			return false;
		};

		auto consumeNumber = [&](double *number = 0)
		{
			if (isEof()) { return false; }

			if (tokens[position].type == Number)
			{
				if (number) { *number = tokens[position].number; }
				position++;
				return true;
			}

			return false;
		};


		if (isEof()) return "";

		if (consumeStringToken("heal"))
		{
			if (!client)
			{
				return "No client was given for the command";
			}

			client->playerData.newLife.life = client->playerData.newLife.maxLife;
			return "Healed";

		}

		if (consumeStringToken("gamemode"))
		{
			if (!client)
			{
				return "No client was given for the command";
			}

			if (consumeStringToken("survival"))
			{
				client->playerData.otherPlayerSettings.gameMode = OtherPlayerSettings::SURVIVAL;
				updateOtherPlayerSettings(*client);

				return "Gamemode set to survival";
			}

			if (consumeStringToken("creative"))
			{
				client->playerData.otherPlayerSettings.gameMode = OtherPlayerSettings::CREATIVE;
				updateOtherPlayerSettings(*client);

				return "Gamemode set to creative";
			}

			return "Invalid command!";
		}

		if (consumeStringToken("difficulty"))
		{
			const auto &difficulty = getServerWorldDifficultySettings();
			return std::string("Difficulty: ") +
				getWorldDifficultyName(difficulty.difficulty) +
				(difficulty.hardcore ? " (Hardcore)" : "");
		}

		if (consumeStringToken("pressure"))
		{
			const auto survivalPlayers = livingSurvivalPlayers();
			const SiegeStatus siegeStatus = getServerSiegeStatus();
			const NaturalSpawnPressure pressure = getNaturalSpawnPressure(
				getServerWorldDayPhase(), getServerVisibleWorldDay(),
				getAllClientsReff().size(), survivalPlayers.size(),
				getServerWorldDifficultySettings(),
				siegeStatus.phase != SiegePhase::Peace);
			if (siegeStatus.phase != SiegePhase::Peace)
			{
				return std::string("World pressure: siege ") +
					getSiegePhaseName(siegeStatus.phase);
			}
			if (!pressure.hostileSpawningEnabled)
			{
				return std::string("World pressure: calm (") +
					(pressure.night ? "night" : "day") + ")";
			}
			return std::string("World pressure: night hostiles ") +
				std::to_string(countNaturalHostiles()) + "/" +
				std::to_string(pressure.hostileCap);
		}

		if (consumeStringToken("siege"))
		{
			if (consumeStringToken("status"))
			{
				const SiegeStatus status = getServerSiegeStatus();
				return std::string("Siege: ") + getSiegePhaseName(status.phase) +
					", wave " + std::to_string(status.currentWave) + "/" +
					std::to_string(status.totalWaves) + ", enemies " +
					std::to_string(status.enemiesRemaining) + ", timer " +
					std::to_string(status.secondsRemaining) + "s, cycles until natural siege " +
					std::to_string(status.cyclesUntilSiege);
			}

			if (consumeStringToken("start"))
			{
				if (commandPermisionLevel < 2) { return "You need admin permission"; }
				return forceServerSiegeWarning() ? "Siege warning started" :
					"A siege is already active";
			}

			return "Usage: /siege status or /siege start";
		}

		if (consumeStringToken("give"))
		{
			if (consumeStringToken("effect"))
			{

				if (!client)
				{
					return "No client was given for the command!";
				}

				for (int i = 0; i < Effects::Effects_Count; i++)
				{
					std::string n(magic_enum::enum_name((Effects::EffectsNames)i).substr());


					if (consumeStringToken(n))
					{
						double number = 0;
						

						if (consumeNumber(&number))
						{

							client->playerData.effects.allEffects[i].timerMs = number * 1000;

							updatePlayerEffects(*client);

							return std::string("Applied effect: ") + n + " for " 
								+ std::to_string(number) + " secconds!";
						}
						else
						{
							return "Invalid command!";
						}


					}

					


				}

				
			}

			return "Invalid command!";
		}



	}

	return "Invalid command!";

}
