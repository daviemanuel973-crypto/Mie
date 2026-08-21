#pragma once

#include <enet/enet.h>
#include <multyPlayer/packet.h>
#include <vector>
#include <unordered_set>
#include <multyPlayer/playerPersistence.h>

struct Client
{
	ENetPeer *peer = {};
	//phisics::Entity entityData = {};
	//bool changed = 1;
	//char clientName[56] = {};
	RevisionNumber revisionNumber = 1;

	PlayerServer playerData;
	PlayerIdentity identity = {};
	bool needsSafeSpawnPlacement = false;

	std::vector<unsigned char> skinData;
	bool skinDataCompressed = false;

	std::unordered_set<glm::ivec2, Ivec2Hash> loadedChunks;
	std::uint64_t lastAcceptedPlayerUpdateMs = 0;
	std::uint64_t lastAcceptedPlayerSimulationMs = 0;

	// Reused navigation field for nearby AI. Rebuilds are throttled in the
	// server tick, avoiding thousands of allocations and a BFS per player/tick.
	PathFindingField navigationField;
	glm::ivec3 navigationOrigin = {};
	std::uint64_t nextNavigationRefreshMs = 0;
	bool hasNavigationOrigin = false;

	std::unordered_set<unsigned int> chunksPacketPendingConfirmation;
};
