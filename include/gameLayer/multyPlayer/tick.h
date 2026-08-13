#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <worldGenerator.h>
#include <multyPlayer/chunkSaver.h>
#include <multyPlayer/serverChunkStorer.h>
#include <random>

bool spawnZombie(ServerChunkStorer &chunkManager, Zombie zombie, std::uint64_t newId);
bool spawnPig(ServerChunkStorer &chunkManager, Pig pig,
	WorldSaver &worldSaver, std::minstd_rand &rng);
bool spawnCat(ServerChunkStorer &chunkManager, Cat cat,
	WorldSaver &worldSaver, std::minstd_rand &rng);
bool spawnGoblin(ServerChunkStorer &chunkManager, Goblin goblin,
	WorldSaver &worldSaver, std::minstd_rand &rng, std::uint64_t *spawnedId = nullptr);

void killEntity(WorldSaver &worldSaver, std::uint64_t entity, ServerChunkStorer &chunkCache);


void entityDeleteFromServerToPlayer(std::uint64_t clientToSend, std::uint64_t eid, 
	bool reliable);

void entityDeleteFromServerToPlayer(Client &client, std::uint64_t eid,
	bool reliable);

void doGameTick(float deltaTime,
	int deltaTimeMs,
	std::uint64_t currentTimer,
	ServerChunkStorer &chunkCache,
	EntityData &orphanEntities,
	unsigned int seed,
	std::vector<ServerTask> waitingTasks, WorldSaver &worldSaver,
	Profiler *profiler
);


void sendDamagePlayerPacket(Client &client);
void sendIncreaseLifePlayerPacket(Client &client);
void sendUpdateLifeLifePlayerPacket(Client &client);

