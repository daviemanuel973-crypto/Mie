#include "multyPlayer/enetServerFunction.h"
#include <atomic>
#include <thread>
#include <enet/enet.h>
#include <iostream>
#include <vector>
#include "multyPlayer/packet.h"
#include <unordered_map>
#include "threadStuff.h"
#include <mutex>
#include <queue>
#include <multyPlayer/server.h>
#include <chrono>
#include <gameplay/entityManagerClient.h>
#include <multyPlayer/server.h>
#include <errorReporting.h>
#include <biome.h>
#include <multyPlayer/chunkSaver.h>
#include <multyPlayer/serverChunkStorer.h>
#include <multyPlayer/serverActionValidation.h>
#include <worldGenerator.h>
#include <fstream>
#include <sstream>
#include <multyPlayer/splitUpdatesLogic.h>
#include <filesystem>
#include <platformTools.h>
#include <profiler.h>
#include <multyPlayer/playerPersistence.h>
#include <gameplay/serverSiegeRuntime.h>
#include <gameplay/worldDifficulty.h>
#include <native/serverNativeSystems.h>
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include <multyPlayer/entityIdAllocator.h>
#include <multyPlayer/dataIntegrity.h>

//todo add to a struct
ENetHost *server = 0;
std::unordered_map<std::uint64_t, Client> connections;
std::unordered_set<ENetPeer *> pendingConnections;
//static std::uint64_t entityId = RESERVED_CLIENTS_ID + 1;
static std::thread enetServerThread;

EntityIdHolder entityIds;
static PersistentEntityIdAllocator persistentEntityIds;
static std::mutex entityIdStateMutex;


std::uint64_t getEntityIdAndIncrement(WorldSaver &worldSaver, int entityType)
{
	std::lock_guard<std::mutex> lock(entityIdStateMutex);
	permaAssert(entityType < EntitiesTypesCount);
	permaAssert(entityType >= 0);

	const std::uint64_t rawId = persistentEntityIds.allocate(worldSaver.savePath,
		static_cast<unsigned int>(entityType));
	permaAssertComment(rawId != 0 && rawId < 0x00FFFFFFFFFFFFFFULL,
		"Server could not reserve a persistent entity ID");
	entityIds.entityIds[entityType] = std::max(entityIds.entityIds[entityType], rawId + 1);
	return rawId | (static_cast<std::uint64_t>(static_cast<unsigned char>(entityType)) << 56);
}

std::uint64_t getCurrentEntityId(int entityType)
{
	std::lock_guard<std::mutex> lock(entityIdStateMutex);
	permaAssert(entityType < EntitiesTypesCount);
	permaAssert(entityType >= 0);
	return std::max(entityIds.entityIds[entityType],
		persistentEntityIds.peek(static_cast<unsigned int>(entityType)));
}

void reserveEntityId(std::uint64_t entityId)
{
	std::lock_guard<std::mutex> lock(entityIdStateMutex);
	const unsigned int entityType = getEntityTypeFromEID(entityId);
	if (entityType >= EntitiesTypesCount) { return; }
	const std::uint64_t rawId = getOnlyIdFromEID(entityId);
	if (rawId >= 0x00FFFFFFFFFFFFFFULL) { return; }

	persistentEntityIds.observe(entityId);
	entityIds.entityIds[entityType] = std::max(entityIds.entityIds[entityType], rawId + 1);
}

void broadCastNotLocked(Packet p, void *data, size_t size, ENetPeer *peerToIgnore, 
	bool reliable, int channel)
{
	for (auto it = connections.begin(); it != connections.end(); it++)
	{
		if (!peerToIgnore || (it->second.peer != peerToIgnore))
		{
			sendPacket(it->second.peer, p, (const char *)data, size, reliable, channel);
		}
	}
}

void broadCast(Packet p, void *data, size_t size, ENetPeer *peerToIgnore, bool reliable, int channel)
{
	broadCastNotLocked(p, data, size, peerToIgnore, reliable, channel);
}

//TODO REMOVE
bool checkIfPlayerShouldGetChunk(glm::ivec2 playerPos2D,
	glm::ivec2 chunkPos, int playerSquareDistance)
{
	glm::ivec2 playerChunk = fromBlockPosToChunkPos({playerPos2D.x, 0, playerPos2D.y});
	float dist = glm::length(glm::vec2(playerChunk - chunkPos));
	if (dist > (playerSquareDistance / 2.f) * std::sqrt(2.f) + 1)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

Client *getClientSafe(std::uint64_t cid)
{
	auto found = connections.find(cid);

	if (found != connections.end())
	{
		return &found->second;
	}

	return nullptr;
}

Client *getClientNotLocked(std::uint64_t cid)
{
	auto it = connections.find(cid);
	if (it == connections.end()) { return 0; }
	return &it->second;
}

std::unordered_map<std::uint64_t, Client> &getAllClientsReff()
{
	return connections;
}

void insertConnection(std::uint64_t cid, Client &c)
{
	connections.insert({cid, c});
}

void sentNewConnectionMessage(ENetPeer *peer, const Client &c, std::uint64_t eid)
{
	Packet p;
	p.header = headerClientRecieveOtherPlayerPosition;
	
	Packet_ClientRecieveOtherPlayerPosition data;
	data.entity = c.playerData.entity;
	data.eid = eid;
	data.timer = getTimer();

	sendPacket(peer, p, (const char *)&data,
		sizeof(data), true, channelHandleConnections);
}

void broadcastNewConnectionMessage(ENetPeer *peerToIgnore, const Client &c, std::uint64_t cid)
{
	Packet p;
	p.header = headerClientRecieveOtherPlayerPosition;

	Packet_ClientRecieveOtherPlayerPosition data;
	data.entity = c.playerData.entity;
	data.eid = cid;

	//no need to lock because this is the thread to modify the data
	broadCastNotLocked(p, &data,
		sizeof(data), peerToIgnore, true, channelHandleConnections);
}

void sendPlayerInventoryAndIncrementRevision(Client &client, int channel)
{
	client.playerData.inventory.revisionNumber++;

	sendPlayerInventoryNotIncrementRevision(client, channel);
}

void sendPlayerInventoryNotIncrementRevision(Client &client, int channel)
{
	std::vector<unsigned char> data;
	client.playerData.inventory.formatIntoData(data);

	//todo adaptive package here, compress
	sendPacket(client.peer, headerClientRecieveAllInventory, data.data(), data.size(), true,
		channel);
}

void sendPlayerExitInteraction(Client &client, unsigned char revisionNumber)
{
	//notify we don't allow interaction!
	Packet packet;
	packet.header = headerRecieveExitBlockInteraction;

	Packet_RecieveExitBlockInteraction packetData;
	packetData.revisionNumber = revisionNumber;

	sendPacket(client.peer, packet,
		(char *)&packetData, sizeof(Packet_RecieveExitBlockInteraction),
		true, channelChunksAndBlocks);
}

namespace
{
	bool blockActionPositionIsValidForClient(const Client &client, const glm::ivec3 &position)
	{
		return mie::serverValidation::isServerBlockActionPositionValid(
			client.playerData.entity.position, position);
	}

	void sendAuthoritativeBlockState(Client &client, const glm::ivec3 &position)
	{
		if (!mie::serverValidation::isWorldBlockPositionSane(position)) { return; }

		Block *block = getServerChunkStorer().getBlockSafe(position);
		if (!block) { return; }

		Packet packet = {};
		packet.header = headerPlaceBlocks;

		Packet_PlaceBlocks packetData = {};
		packetData.blockPos = position;
		packetData.blockInfo = *block;

		sendPacket(client.peer, packet,
			reinterpret_cast<const char *>(&packetData), sizeof(packetData),
			true, channelChunksAndBlocks);
	}

	void rejectBlockMutation(Client &client, const EventId &eventId,
		const glm::ivec3 &position, bool resyncInventory)
	{
		computeRevisionStuff(client, false, eventId);
		if (resyncInventory)
		{
			sendPlayerInventoryAndIncrementRevision(client);
		}
		sendAuthoritativeBlockState(client, position);
	}

	bool isSpawnEggItem(unsigned short itemType)
	{
		return itemType == ItemTypes::pigSpawnEgg ||
			itemType == ItemTypes::zombieSpawnEgg ||
			itemType == ItemTypes::catSpawnEgg ||
			itemType == ItemTypes::goblinSpawnEgg ||
			itemType == ItemTypes::scareCrowSpawnEgg;
	}
}

void updatePlayerEffects(Client &client)
{
	Packet packet;
	packet.header = headerUpdateEffects;

	Packet_UpdateEffects packetData;
	packetData.timer = getTimer();
	packetData.effects = client.playerData.effects;

	sendPacket(client.peer, packet,
		(char *)&packetData, sizeof(packetData),
		true, channelEffects);
}

void updatePlayerSurvivalStats(Client &client)
{
	client.playerData.survivalStats.sanitize();
	Packet_UpdateSurvivalStats packetData;
	packetData.stats = client.playerData.survivalStats;
	sendPacket(client.peer, headerUpdateSurvivalStats, &packetData, sizeof(packetData), true, channelEffects);
}

void addConnection(ENetHost *server, ENetEvent &event, WorldSaver &worldSaver)
{
	event.peer->timeoutMinimum = 10'000;
	event.peer->timeoutMaximum = 30'000;
	event.peer->timeoutLimit = 64;
	pendingConnections.insert(event.peer);
}

bool isIdentityAlreadyConnected(const PlayerIdentity &identity)
{
	for (const auto &connection : connections)
	{
		if (connection.second.identity == identity) { return true; }
	}
	return false;
}

void finishAddingConnection(ENetEvent &event, WorldSaver &worldSaver,
	const PlayerIdentity &identity)
{
	if (!identity.isValid() || isIdentityAlreadyConnected(identity))
	{
		std::cout << "Rejected duplicate or invalid player identity.\n";
		pendingConnections.erase(event.peer);
		enet_peer_disconnect(event.peer, 0);
		return;
	}

	std::uint64_t id = getEntityIdAndIncrement(worldSaver, EntityType::player);

	{
		Client c{event.peer};
		c.identity = identity;
		glm::dvec3 safeSpawn = glm::dvec3(worldSaver.spawnPosition);
		const bool resolvedSafeSpawn = tryResolveSafeServerSpawn(worldSaver, safeSpawn);
		c.playerData.entity.position = safeSpawn;
		c.playerData.entity.lastPosition = safeSpawn;

		// Survival is now the default. Creative remains available through the server command.
		c.playerData.otherPlayerSettings.gameMode = OtherPlayerSettings::SURVIVAL;
		c.playerData.inventory = PlayerInventory{};
		c.playerData.survivalStats = {};
		const bool restored = loadPlayerFromDisk(worldSaver.savePath, identity, c.playerData);
		c.needsSafeSpawnPlacement = !restored && !resolvedSafeSpawn;
		c.playerData.lastChunkPositionWhenAnUpdateWasSent.x = divideChunk(c.playerData.entity.position.x);
		c.playerData.lastChunkPositionWhenAnUpdateWasSent.y = divideChunk(c.playerData.entity.position.z);
		std::cout << (restored ? "Restored" : "Created") << " player "
			<< identity.toString() << ".\n";

		insertConnection(id, c);
	}
	pendingConnections.erase(event.peer);

	{
		Packet p;
		p.header = headerReceiveCIDAndData;
		p.cid = id;
		const Client *connectedClient = getClientNotLocked(id);
		if (!connectedClient) { return; }

		Packet_ReceiveCIDAndData packetToSend = {};
		packetToSend.entity = connectedClient->playerData.entity;
		packetToSend.yourPlayerEntityId = id;
		packetToSend.timer = getTimer();
		packetToSend.otherSettings = connectedClient->playerData.otherPlayerSettings;
		packetToSend.survivalStats = connectedClient->playerData.survivalStats;

		//send own cid
		sendPacket(event.peer, p, (const char *)&packetToSend,
			sizeof(packetToSend), true, channelHandleConnections);


	}

	{
		const auto &settings = getServerWorldDifficultySettings();
		Packet_UpdateWorldDifficulty packetData;
		packetData.difficulty = static_cast<std::uint8_t>(settings.difficulty);
		packetData.hardcore = settings.hardcore ? 1u : 0u;
		sendPacket(event.peer, headerUpdateWorldDifficulty, &packetData,
			sizeof(packetData), true, channelHandleConnections);
	}

	//todo maybe send entities to this new connection?


	{
		auto timer = getTimer();

		Packet packet;
		packet.header = headerClientUpdateTimer;

		Packet_ClientUpdateTimer packetData;
		packetData.timer = timer;

		sendPacket(event.peer, packet, (const char *)&packetData,
			sizeof(packetData), true, channelHandleConnections);
	}

	//send inventory
	{
		sendPlayerInventoryAndIncrementRevision(connections[id], channelHandleConnections);
	}

	//send others to this
	for (auto &c : connections)
	{
		if (c.first != id)
		{
			sentNewConnectionMessage(event.peer, c.second, c.first);

			//send skin
			{
				Packet p;
				p.header = headerSendPlayerSkin;
				p.cid = c.first;
					
				if (c.second.skinDataCompressed)
				{
					p.setCompressed();
				}

				sendPacket(event.peer, p, (const char*)c.second.skinData.data(),
					c.second.skinData.size(), true, channelHandleConnections);
			}

		}

	}

	addCidToServerSettings(id);

	//send this to others
	if (const Client *connectedClient = getClientNotLocked(id))
	{
		broadcastNewConnectionMessage(event.peer, *connectedClient, id);
	}

}

bool saveConnection(const Client &client, const WorldSaver &worldSaver)
{
	if (savePlayerToDisk(worldSaver.savePath, client.identity, client.playerData)) { return true; }
	std::cerr << "Warning: could not save player " << client.identity.toString() << ".\n";
	return false;
}

void saveAllConnections(const WorldSaver &worldSaver)
{
	for (const auto &connection : connections)
	{
		saveConnection(connection.second, worldSaver);
	}
}

void removeConnection(ENetHost *server, ENetEvent &event, WorldSaver &worldSaver)
{
	pendingConnections.erase(event.peer);

	for (auto connection = connections.begin(); connection != connections.end(); ++connection)
	{
		if (connection->second.peer == event.peer)
		{
			Packet p = {};
			p.header = headerDisconnectOtherPlayer;
			p.cid = connection->first;

			Packet_DisconectOtherPlayer data;
			data.EID = connection->first;

			broadCastNotLocked(p, &data, sizeof(data), 0, true, channelHandleConnections);
			saveConnection(connection->second, worldSaver);
			removeCidFromServerSettings(connection->first);
			connections.erase(connection);
			break;
		}
	}


}

void recieveData(ENetHost *server, ENetEvent &event, std::vector<ServerTask> &serverTasks,
	WorldSaver &worldSaver)
{


	Packet p;
	size_t size = 0;
	auto data = parsePacket(event, p, size);
	bool wasCompressed = false;

	if (p.header == headerClientIdentity && pendingConnections.find(event.peer) != pendingConnections.end())
	{
		if (!p.isCompressed() && data && size == sizeof(Packet_ClientIdentity))
		{
			const auto &identityPacket = *reinterpret_cast<const Packet_ClientIdentity *>(data);
			if (identityPacket.protocolVersion == MULTIPLAYER_PROTOCOL_VERSION)
			{
				finishAddingConnection(event, worldSaver, identityPacket.identity);
				return;
			}
		}

		std::cout << "Rejected invalid player identity handshake.\n";
		pendingConnections.erase(event.peer);
		enet_peer_disconnect(event.peer, 0);
		return;
	}

	if (pendingConnections.find(event.peer) != pendingConnections.end())
	{
		std::cout << "Rejected data received before player identity handshake.\n";
		pendingConnections.erase(event.peer);
		enet_peer_disconnect(event.peer, 0);
		return;
	}

	if (p.isCompressed())
	{
		p.setNotCompressed();
		if (p.header != headerSendPlayerSkin)
		{
			reportError("Rejected unsupported compressed client packet.");
			enet_peer_disconnect(event.peer, 0);
			return;
		}

		wasCompressed = true;
	}

	auto connection = connections.find(p.cid);

	if (connection == connections.end())
	{
		reportError((std::string("packet recieved with a CID that doesn't exist: ") + std::to_string(p.cid) + "\n").c_str());
		return;
	}

	//validate data
	//no need for mutex here fortunatelly because this thread is the one that modifies the connections
	//connectionsMutex.lock();
	if (connection->second.peer != event.peer)
	{
		//connectionsMutex.unlock();
		reportError("invalidData: connections[p.cid].peer != event.peer");
		return;
	}
	//connectionsMutex.unlock();


	ServerTask serverTask = {};
	serverTask.cid = p.cid;


	switch (p.header)
	{

		//todo hard reset on fail

		case headerClientDroppedChunk:
		{
			if (!data || size != sizeof(Packet_ClientDroppedChunk))
			{
				reportError("corrupted packet or something Packet_ClientDroppedChunk");
				break;
			}

			const Packet_ClientDroppedChunk packetData =
				*reinterpret_cast<const Packet_ClientDroppedChunk *>(data);
			connection->second.loadedChunks.erase(packetData.chunkPos);

			break;
		}

		case headerClientDroppedAllChunks:
		{
			if (size != 0) { break; }
			connection->second.loadedChunks.clear();

			break;
		}

		case headerPlaceBlock:
		{
			if (!data || size != sizeof(Packet_ClientPlaceBlock))
			{
				reportError("corrupted packet or something Packet_ClientPlaceBlock");
				break;
			}

			const Packet_ClientPlaceBlock packetData =
				*reinterpret_cast<const Packet_ClientPlaceBlock *>(data);
			if (!blockActionPositionIsValidForClient(connection->second, packetData.blockPos))
			{
				rejectBlockMutation(connection->second, packetData.eventId,
					packetData.blockPos, true);
				break;
			}

			serverTask.t.taskType = Task::placeBlock;
			serverTask.t.pos = {packetData.blockPos};
			serverTask.t.blockType = {packetData.blockType};
			serverTask.t.eventId = packetData.eventId;
			serverTask.t.revisionNumber = packetData.inventoryRevision;
			serverTask.t.inventroySlot = packetData.inventorySlot;

			serverTasks.push_back(serverTask);
			break;
		}

		case headerPlaceBlockForce:
		{
			if (!data || size != sizeof(Packet_ClientPlaceBlockForce))
			{
				reportError("corrupted packet or something Packet_ClientPlaceBlockForce");
				break;
			}

			const Packet_ClientPlaceBlockForce packetData =
				*reinterpret_cast<const Packet_ClientPlaceBlockForce *>(data);
			if (!blockActionPositionIsValidForClient(connection->second, packetData.blockPos))
			{
				rejectBlockMutation(connection->second, packetData.eventId,
					packetData.blockPos, false);
				break;
			}

			serverTask.t.taskType = Task::placeBlockForce;
			serverTask.t.pos = packetData.blockPos;
			serverTask.t.block = packetData.block;
			serverTask.t.eventId = packetData.eventId;

			serverTasks.push_back(serverTask);
			break;
		}

		case headerBreakBlock:
		{
			if (!data || size != sizeof(Packet_ClientBreakBlock))
			{
				reportError("corrupted packet or something Packet_ClientBreakBlock");
				break;
			}

			const Packet_ClientBreakBlock packetData =
				*reinterpret_cast<const Packet_ClientBreakBlock *>(data);
			if (!blockActionPositionIsValidForClient(connection->second, packetData.blockPos))
			{
				rejectBlockMutation(connection->second, packetData.eventId,
					packetData.blockPos, true);
				break;
			}

			serverTask.t.taskType = Task::breakBlock;
			serverTask.t.pos = {packetData.blockPos};
			serverTask.t.eventId = packetData.eventId;
			serverTask.t.revisionNumber = packetData.inventoryRevision;
			serverTask.t.inventroySlot = packetData.inventorySlot;

			serverTasks.push_back(serverTask);
			break;

		}

		case headerSendPlayerData:
		{
			if (!data || size != sizeof(Packer_SendPlayerData))
			{
				reportError("corrupted packet or something Packer_SendPlayerData");
				break;
			}

			Packer_SendPlayerData packetData = {};
			memcpy(&packetData, data, sizeof(packetData));
			const auto &position = packetData.playerData.position;
			if (!mie::dataIntegrity::isFiniteWorldPosition(position) ||
				!mie::dataIntegrity::isDroppedItemMotionStateValid(packetData.playerData.forces))
			{
				reportError("Rejected player update with invalid transform or motion state.");
				break;
			}
			const std::uint64_t now = getTimer();
			const bool timestampTooOld = packetData.timer < now && now - packetData.timer > 1000;
			const bool timestampTooFarAhead = packetData.timer > now + 2000;
			const bool timestampDidNotAdvance = connection->second.lastAcceptedPlayerSimulationMs != 0 &&
				packetData.timer <= connection->second.lastAcceptedPlayerSimulationMs;
			const bool updateRateExceeded = connection->second.lastAcceptedPlayerUpdateMs != 0 &&
				now - connection->second.lastAcceptedPlayerUpdateMs < 12;
			if (timestampTooOld || timestampTooFarAhead || timestampDidNotAdvance || updateRateExceeded)
			{
				break;
			}
			connection->second.lastAcceptedPlayerUpdateMs = now;
			connection->second.lastAcceptedPlayerSimulationMs = packetData.timer;
			// Bound client-controlled streaming work. A forged 64-chunk request can
			// otherwise cause a large CPU/RAM spike on the server.
			packetData.playerData.chunkDistance = std::clamp(packetData.playerData.chunkDistance, 2, 24);

			ENetPeer *peerToIgnore = nullptr;
			std::uint64_t clientCopyCid = 0;
		
			{

				if (connection->second.playerData.entity.position != packetData.playerData.position)
				{

					peerToIgnore = connection->second.peer;
					clientCopyCid = connection->first;

					//todo something better here...
					auto s = getServerSettingsCopy();
					s.perClientSettings[p.cid].outPlayerPos = packetData.playerData.position;
					setServerSettings(s);
				}
				else
				{
					//nothing changed
				}

				connection->second.playerData.entity = packetData.playerData;

			}

			if (clientCopyCid)
			{
				Packet_ClientRecieveOtherPlayerPosition sendData;

				sendData.timer = now;
				sendData.eid = clientCopyCid;
				sendData.entity = packetData.playerData;

				Packet p;
				p.cid = 0;
				p.header = headerClientRecieveOtherPlayerPosition;

				const glm::ivec2 playerChunk = determineChunkThatIsEntityIn(
					packetData.playerData.position);
				for (auto &recipient : connections)
				{
					if (recipient.second.peer == peerToIgnore ||
						recipient.second.loadedChunks.find(playerChunk) == recipient.second.loadedChunks.end())
					{
						continue;
					}
					sendPacket(recipient.second.peer, p,
						reinterpret_cast<const char *>(&sendData), sizeof(sendData),
						false, channelPlayerPositions);
				}
			}

			break;
		}

		//todo inventory revision here
		case headerClientDroppedItem:
		{
			if (!data || size != sizeof(Packet_ClientDroppedItem))
			{
				//error checking + kick clients that send corrupted data? hard reset
				reportError("corrupted packet or something Packet_ClientDroppedItem");
				break;
			}

			Packet_ClientDroppedItem *packetData = (Packet_ClientDroppedItem *)data;

			serverTask.t.taskType = Task::droppedItemEntity;
			serverTask.t.doublePos = packetData->position;
			serverTask.t.blockCount = packetData->count;
			serverTask.t.from = packetData->inventorySlot;
			serverTask.t.entityId = packetData->entityID;
			serverTask.t.eventId = packetData->eventId;
			serverTask.t.motionState = packetData->motionState;
			serverTask.t.timer = packetData->timer;
			serverTask.t.blockType = packetData->type;
			serverTask.t.revisionNumber = packetData->revisionNumberInventory;

			serverTasks.push_back(serverTask);
			break;

		}

		//inventory stuff
		case headerClientMovedItem:
		{
			if (!data || size != sizeof(Packet_ClientMovedItem))
			{
				//todo hard reset on errors.
				reportError("corrupted packet or something Packet_ClientMovedItem");
				break;
			}
			Packet_ClientMovedItem *packetData = (Packet_ClientMovedItem *)data;

			serverTask.t.taskType = Task::clientMovedItem;
			serverTask.t.itemType = packetData->itemType;
			serverTask.t.from = packetData->from;
			serverTask.t.to = packetData->to;
			serverTask.t.blockCount = packetData->counter;
			serverTask.t.revisionNumber = packetData->revisionNumber;
			serverTasks.push_back(serverTask);

			break;
		}

		case headerClientCraftedItem:
		{

			if (!data || size != sizeof(Packet_ClientCraftedItem))
			{
				//todo hard reset on errors.
				reportError("corrupted packet or something Packet_ClientCraftedItem");
				break;
			}
			Packet_ClientCraftedItem *packetData = (Packet_ClientCraftedItem *)data;

			serverTask.t.taskType = Task::clientCraftedItem;
			serverTask.t.craftingRecepieIndex = packetData->recepieIndex;
			serverTask.t.to = packetData->to;
			serverTask.t.revisionNumber = packetData->revisionNumber;
			serverTasks.push_back(serverTask);

			break;
		}

		case headerClientOverWriteItem:
		{
			if (!data || size < sizeof(Packet_ClientOverWriteItem))
			{
				break;
			}

			Packet_ClientOverWriteItem *packetData = (Packet_ClientOverWriteItem *)data;
			serverTask.t.taskType = Task::clientOverwriteItem;
			serverTask.t.itemType = packetData->itemType;
			serverTask.t.to = packetData->to;
			serverTask.t.blockCount = packetData->counter;
			serverTask.t.revisionNumber = packetData->revisionNumber;

			int metaDataSize = packetData->metadataSize;

			if (size - sizeof(Packet_ClientOverWriteItem) != static_cast<size_t>(metaDataSize))
			{
				//todo hard reset on errors.
				break;
			}

			serverTask.t.metaData.resize(metaDataSize);
			memcpy(serverTask.t.metaData.data(), data + sizeof(Packet_ClientOverWriteItem), metaDataSize);

			serverTasks.push_back(std::move(serverTask));

			break;
		}

		case headerClientSwapItems:
		{
			if (!data || size != sizeof(Packet_ClientSwapItems))
			{
				break; //todo hard reset stuff everywhere
			}

			Packet_ClientSwapItems *packetData = (Packet_ClientSwapItems *)data;
			serverTask.t.taskType = Task::clientSwapItems;
			serverTask.t.from = packetData->from;
			serverTask.t.to = packetData->to;
			serverTask.t.revisionNumber = packetData->revisionNumber;
			serverTasks.push_back(serverTask);

			break;

		}

		//todo tick timer here
		case headerClientUsedItem:
		{

			if (!data || size != sizeof(Packet_ClientUsedItem))
			{
				break;
			}

			Packet_ClientUsedItem *packetData = (Packet_ClientUsedItem *)data;
			Item requestedItem(packetData->itemType);
			const bool mutatesBlock = requestedItem.isPaint();
			const bool positionalServerUse = mutatesBlock || isSpawnEggItem(packetData->itemType);
			if (positionalServerUse &&
				!blockActionPositionIsValidForClient(connection->second, packetData->position))
			{
				if (mutatesBlock)
				{
					rejectBlockMutation(connection->second, packetData->eventId,
						packetData->position, true);
				}
				else
				{
					sendPlayerInventoryNotIncrementRevision(connection->second);
				}
				break;
			}

			serverTask.t.taskType = Task::clientUsedItem;
			serverTask.t.from = packetData->from;
			serverTask.t.itemType = packetData->itemType;
			serverTask.t.pos = packetData->position;
			serverTask.t.revisionNumber = packetData->revisionNumber;
			serverTask.t.eventId = packetData->eventId;
			serverTasks.push_back(serverTask);

			break;
		}
		
		case headerClientInteractWithBlock:
		{

			if (!data || size != sizeof(Packet_ClientInteractWithBlock))
			{
				break;
			}

			Packet_ClientInteractWithBlock *packetData = (Packet_ClientInteractWithBlock *)data;
			if (!blockActionPositionIsValidForClient(connection->second, packetData->blockPos))
			{
				sendPlayerExitInteraction(connection->second, packetData->interactionCounter);
				break;
			}

			serverTask.t.taskType = Task::clientInteractedWithBlock;
			serverTask.t.blockType = packetData->blockType;
			serverTask.t.pos = packetData->blockPos;
			serverTask.t.revisionNumber = packetData->interactionCounter;
			serverTasks.push_back(serverTask);

			break;
		}

		case headerRecieveExitBlockInteraction:
		{
			if (!data || size != sizeof(Packet_RecieveExitBlockInteraction))
			{
				break;
			}

			Packet_RecieveExitBlockInteraction *packetData = (Packet_RecieveExitBlockInteraction *)data;
			serverTask.t.taskType = Task::clientExitedInteractionWithBlock;
			serverTask.t.revisionNumber = packetData->revisionNumber;
			
			serverTasks.push_back(serverTask);

		}
		break;

		case headerSendPlayerSkin:
		{
			const std::size_t rawSkinSize = 4u * PLAYER_SKIN_SIZE * PLAYER_SKIN_SIZE;
			if (!data || size > rawSkinSize || (!wasCompressed && size != rawSkinSize)) { break; }
			if (wasCompressed)
			{
				std::size_t decompressedSize = 0;
				void *verifiedSkin = unCompressDataBounded(data, size, decompressedSize, rawSkinSize);
				if (!verifiedSkin || decompressedSize != rawSkinSize)
				{
					delete[] static_cast<char *>(verifiedSkin);
					break;
				}
				delete[] static_cast<char *>(verifiedSkin);
			}

			connection->second.skinData.resize(size);
			memcpy(connection->second.skinData.data(), data, size);
			connection->second.skinDataCompressed = wasCompressed;

			{

				Packet p;
				p.header = headerSendPlayerSkin;
				p.cid = serverTask.cid;

				auto client = getClientNotLocked(serverTask.cid);

				if (client)
				{
					if (client->skinDataCompressed)
					{
						p.setCompressed();
					}

					broadCastNotLocked(p, client->skinData.data(),
						client->skinData.size(), client->peer, true, channelHandleConnections);
				}
			}

		}
		break;

		case headerAttackEntity:
		{
			if (!data || size != sizeof(Packet_AttackEntity))
			{
				break;
			}

			Packet_AttackEntity packetData = {};
			memcpy(&packetData, data, sizeof(packetData));
			serverTask.t.taskType = Task::clientAttackedEntity;
			serverTask.t.entityId = packetData.entityID;
			serverTask.t.inventroySlot = packetData.inventorySlot;
			serverTask.t.revisionNumber = packetData.inventoryRevision;
			serverTask.t.vector = packetData.direction;
			serverTask.t.hitResult = packetData.hitResult;

			//early ignore useless packets
			const glm::vec3 attackDirection = serverTask.t.vector;
			const float directionLengthSquared = glm::dot(attackDirection, attackDirection);
			if (!std::isfinite(serverTask.t.hitResult.hitCorectness) ||
				!std::isfinite(serverTask.t.hitResult.bonusCritChance) ||
				!std::isfinite(attackDirection.x) || !std::isfinite(attackDirection.y) ||
				!std::isfinite(attackDirection.z) || !std::isfinite(directionLengthSquared) ||
				directionLengthSquared <= 0.000001f || directionLengthSquared > 4.f ||
				serverTask.t.hitResult.hitCorectness > 1 ||
				serverTask.t.hitResult.hitCorectness < 0 ||
				serverTask.t.hitResult.bonusCritChance > 1 ||
				serverTask.t.hitResult.bonusCritChance < -1 ||
				serverTask.t.hitResult.hit == 0
				)
			{
				break;
			}
			serverTask.t.vector = glm::normalize(attackDirection);

			serverTasks.push_back(serverTask);
			break;
		}
		
		case headerClientWantsToRespawn:
		{
			if (size != 0)
			{
				break;
			}
			serverTask.t.taskType = Task::clientWantsToRespawn;
			serverTasks.push_back(serverTask);
			break;
		}

		case headerClientDamageLocally:
		{
			if (!data || size != sizeof(Packet_ClientDamageLocally))
			{
				break;
			}

			Packet_ClientDamageLocally *packetData = (Packet_ClientDamageLocally *)data;
			if (packetData->damage <= 0 || packetData->damage > 1000) { break; }

			serverTask.t.taskType = Task::clientRecievedDamageLocally;
			serverTask.t.damage = packetData->damage;
			serverTasks.push_back(serverTask);
			break;
		}

		case headerClientDamageLocallyAndDied:
		{
			if (size != 0)
			{
				break;
			}
			serverTask.t.taskType = Task::clientRecievedDamageLocallyAndDied;
			serverTasks.push_back(serverTask);
			break;
		}

		case headerSendChat:
		{

			if (!data || size == 0) { break; }
			if (size > 260) { break; } //we ignore messages that are too big.
			data[size - 1] = 0; //making sure the packet is null terminated!

			if (size > 1 && data[0] == '/')
			{
				auto rez = executeServerCommand(p.cid, data + 1);

				Packet newPacket;
				newPacket.cid = 0;
				newPacket.header = headerSendChat;

				sendPacket(connection->second.peer, newPacket, rez.c_str(),
					rez.size() + 1, true, channelHandleConnections);
			}
			else
			{
				Packet newPacket;
				newPacket.cid = p.cid;
				newPacket.header = headerSendChat;

				broadCast(newPacket, data, size, nullptr, true,
					channelHandleConnections);
			}

			break;
		}

		case headerClientChangeBlockData:
		{
			if (!data || size < sizeof(Packet_ClientChangeBlockData)) { break; }

			Packet_ClientChangeBlockData *blockData = (Packet_ClientChangeBlockData*)data;
			if (blockData->blockDataHeader.dataSize != size - sizeof(Packet_ClientChangeBlockData)) { break; }
			if (!blockActionPositionIsValidForClient(connection->second, blockData->blockDataHeader.pos))
			{
				rejectBlockMutation(connection->second, blockData->eventId,
					blockData->blockDataHeader.pos, false);
				break;
			}

			serverTask.t.taskType = Task::clientChangedBlockData;
			serverTask.t.eventId = blockData->eventId;
			serverTask.t.blockType = blockData->blockDataHeader.blockType;
			serverTask.t.pos = blockData->blockDataHeader.pos;
			serverTask.t.metaData.resize(blockData->blockDataHeader.dataSize);
			memcpy(serverTask.t.metaData.data(), data + sizeof(Packet_ClientChangeBlockData), blockData->blockDataHeader.dataSize);
			serverTasks.push_back(serverTask);

			break;
		}

		default:

		break;
	}





}



static std::atomic<bool> enetServerRunning = false;


Profiler serverProfiler;

//used for accessing the profiler from another thread
Profiler getServerProfilerCopy()
{
	return serverProfiler;
}


int pendingReliableCount = 0;
size_t totalSize = 0;

int getServerPendingReliableCount()
{
	return pendingReliableCount;
}

size_t getServerTotalPendingSize()
{
	return totalSize;
}

void calculatePendingPacketsMetrics()
{
	pendingReliableCount = 0;
	totalSize = 0;

	for (auto &c : connections)
	{
		if (!c.second.peer)continue;

		auto peer = c.second.peer;

		enet_uint16 lastSent = peer->outgoingReliableSequenceNumber;
		ENetListNode *current = peer->sentReliableCommands.sentinel.next;
		while (current != &peer->sentReliableCommands.sentinel)
		{
			ENetOutgoingCommand *command = (ENetOutgoingCommand *)current;

			if (command->reliableSequenceNumber > 0)
			{
				++pendingReliableCount;
			}

			current = current->next;
		}

		totalSize += peer->totalWaitingData;
	}

}




void enetServerFunction(std::string path)
{
	std::cout << "Successfully started server!\n";

	//todo load from file or something
	entityIds.create();

	StructuresManager structuresManager;
	BiomesManager biomesManager;
	WorldSaver worldSaver;
	serverProfiler = Profiler{};

	worldSaver.savePath = USER_CONTENT_PATH "worlds/";
	worldSaver.savePath += path + "/world";
	WorldDifficultySettings difficultySettings;
	const auto worldRoot = std::filesystem::path(USER_CONTENT_PATH "worlds/") / path;
	const auto difficultyPath = worldRoot / "worldDifficulty";
	if (!loadWorldDifficultySettings(worldRoot, difficultySettings) &&
		std::filesystem::exists(difficultyPath))
	{
		std::cerr << "Warning: invalid world difficulty data; using Normal.\n";
		difficultySettings = {};
	}
	setServerWorldDifficultySettings(difficultySettings);
	configureServerSiegeDifficulty(getSiegeEnemyMultiplier(difficultySettings),
		areNaturalSiegesEnabled(difficultySettings));


	{
		std::error_code err = {};
		std::filesystem::create_directory(worldSaver.savePath, err);
		if (err) 
		{ 
			std::cout << err << "\n";
			exit(0); 
		}
	}
	loadServerSiegeRuntime(worldSaver);
	mie::native::loadServerNativeSystems(worldSaver);
	
	if (!structuresManager.loadAllStructures())
	{
		exit(0);
	}
	if (!biomesManager.loadAllBiomes())
	{
		exit(0);
	}

	WorldGenerator wg;
	wg.init();
	ENetEvent event = {};


	std::ifstream seedFile(std::string(USER_CONTENT_PATH "worlds/") + path + "/seed.txt");
	if (!seedFile.is_open())
	{
		std::ifstream worldSettingsFile(std::string(USER_CONTENT_PATH "worlds/") + path + "/worldGenSettings.wgenerator");
		
		if (worldSettingsFile.is_open())
		{
			std::stringstream buffer;
			buffer << worldSettingsFile.rdbuf();
			WorldGeneratorSettings s;
			if (s.loadSettings(buffer.str().c_str()))
			{
				s.sanitize();
				wg.applySettings(s);
			}
			else
			{
				std::cout << "NOISE LOADING ERROR";
				exit(0);
			}
			worldSettingsFile.close();
		}

	}
	else
	{
		int seed = 0;
		seedFile >> seed;
		if (!seedFile)
		{
			exit(0);
		}
		seedFile.close();


		std::ifstream f(RESOURCES_PATH "gameData/worldGenerator/default.wgenerator");
		if (f.is_open())
		{
			std::stringstream buffer;
			buffer << f.rdbuf();
			WorldGeneratorSettings s;
			if (s.loadSettings(buffer.str().c_str()))
			{
				s.seed = seed;
				wg.applySettings(s);
			}
			else
			{
				std::cout << "NOISE LOADING ERROR";
				exit(0);
			}
			f.close();
		}
		else
		{
			exit(0);
		}
	}


	auto start = std::chrono::high_resolution_clock::now();

	float sendEntityTimer = 0.5;
	float sentTimerUpdateTimer = 1;
	float playerAutosaveTimer = 30;

	float tickTimer = 0;

	std::vector<ServerTask> serverTasks;
	serverTasks.reserve(100);


	serverWorkerUpdate(wg, structuresManager, biomesManager,
		worldSaver, serverTasks, (1.f/targetTicksPerSeccond) + 0.01, serverProfiler);

	while (enetServerRunning)
	{
		auto stop = std::chrono::high_resolution_clock::now();

		float deltaTime = (std::chrono::duration_cast<std::chrono::microseconds>(stop - start)).count() / 1000000.0f;
		start = std::chrono::high_resolution_clock::now();
		tickTimer += deltaTime;
		playerAutosaveTimer -= deltaTime;

		serverProfiler.startFrame();

		auto settings = getServerSettingsCopy();
		
		serverProfiler.startSubProfile("Recieve Network Updates");
		int waitTime = 1;
		int tries = 10;
		while (((enet_host_service(server, &event, waitTime) > 0) || (waitTime=0, tries-- > 0) ) 
			&& enetServerRunning)
		{
			waitTime = 0;

			switch (event.type)
			{
				case ENET_EVENT_TYPE_CONNECT:
				{
					addConnection(server, event, worldSaver);

					std::cout << "Successfully connected!\n";

					break;
				}
				case ENET_EVENT_TYPE_RECEIVE:
				{
					recieveData(server, event, serverTasks, worldSaver);

					enet_packet_destroy(event.packet);

					break;
				}
				case ENET_EVENT_TYPE_DISCONNECT:
				{

					std::cout << "disconnect from server: "
						<< event.peer->address.host << " "
						<< event.peer->address.port << "\n\n";
					removeConnection(server, event, worldSaver);
					break;
				}


			}
		}
		serverProfiler.endSubProfile("Recieve Network Updates");

		if (playerAutosaveTimer <= 0)
		{
			playerAutosaveTimer = 30;
			saveAllConnections(worldSaver);
			if (!saveServerSiegeRuntime(worldSaver))
			{
				std::cerr << "Warning: could not autosave world time and siege schedule.\n";
			}
			if (!mie::native::saveServerNativeSystems(worldSaver))
			{
				std::cerr << "Warning: could not autosave v0.6 native systems.\n";
			}
		}


		if (!enetServerRunning) { break; }


	#pragma region server sends timer updates
		{

			sentTimerUpdateTimer -= deltaTime;
			if (sentTimerUpdateTimer < 0)
			{
				sentTimerUpdateTimer = 1.f;
				auto timer = getTimer();
			
				Packet packet;
				packet.header = headerClientUpdateTimer;
					
				Packet_ClientUpdateTimer packetData;
				packetData.timer = timer;

				broadCast(packet, &packetData, sizeof(packetData), nullptr, false, channelHandleConnections);

			}

		}
	#pragma endregion

		serverWorkerUpdate(wg, structuresManager, biomesManager,
			worldSaver, serverTasks, deltaTime, serverProfiler);

		calculatePendingPacketsMetrics();

		serverProfiler.endFrame();
	}

	saveAllConnections(worldSaver);
	if (!saveServerSiegeRuntime(worldSaver))
	{
		std::cerr << "Warning: could not save world time and siege schedule during shutdown.\n";
	}
	if (!mie::native::saveServerNativeSystems(worldSaver, true))
	{
		std::cerr << "Warning: could not save v0.6 native systems during shutdown.\n";
	}
	clearSD(worldSaver);
	wg.clear();
	structuresManager.clear();

	for (auto &c : connections)
	{
		enet_peer_disconnect(c.second.peer, 0);
		enet_peer_reset(c.second.peer);
	}

	enet_host_flush(server);

	int capCounter = 20;
	while (enet_host_service(server, &event, 100) > 0 && capCounter-->0)
	{
		if (event.type == ENET_EVENT_TYPE_RECEIVE)
		{
			enet_packet_destroy(event.packet);
		}
	}

}

bool startEnetListener(ENetHost *_server, const std::string &path)
{
	server = _server;
	connections = {};
	pendingConnections = {};

	bool expected = 0;
	if (enetServerRunning.compare_exchange_strong(expected, 1))
	{
		enetServerThread = std::move(std::thread(enetServerFunction, path));
		return 1;
	}
	else
	{
		return 0;
	}
}

void closeEnetListener()
{
	enetServerRunning = false;
	enetServerThread.join();
}
