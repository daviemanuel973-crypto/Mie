#pragma once

#include <cstdint>
#include <random>

#include <gameplay/siege.h>

struct ServerChunkStorer;
struct WorldSaver;

void resetServerSiegeRuntime();
void configureServerSiegeDifficulty(float enemyCountMultiplier,
	bool naturalSiegesEnabled);
bool loadServerSiegeRuntime(const WorldSaver &worldSaver);
bool saveServerSiegeRuntime(const WorldSaver &worldSaver);
void updateServerSiegeRuntime(float deltaTime, ServerChunkStorer &chunkStorer,
	WorldSaver &worldSaver, std::minstd_rand &rng);
SiegeStatus getServerSiegeStatus();
bool isServerSiegeWaveActive();
bool isServerSiegeEnemy(std::uint64_t entityId);
bool forceServerSiegeWarning();
