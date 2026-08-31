#include <multyPlayer/packetValidation.h>

#include <gameplay/fieldGuide.h>
#include <gameplay/fieldGuideProtocol.h>
#include <multyPlayer/packet.h>

#include <cstring>

namespace
{
	template <class T>
	PacketPayloadRule exactRule()
	{
		return {sizeof(T), sizeof(T), 1};
	}

	bool validateBlockDataRecords(const char *data, std::size_t size)
	{
		std::size_t cursor = 0;
		while (cursor < size)
		{
			if (size - cursor < sizeof(BlockDataHeader)) { return false; }
			BlockDataHeader header = {};
			std::memcpy(&header, data + cursor, sizeof(header));
			cursor += sizeof(header);
			if (header.dataSize > size - cursor) { return false; }
			cursor += header.dataSize;
		}
		return cursor == size;
	}

	bool validateSingleBlockDataRecord(const char *data, std::size_t size)
	{
		if (size < sizeof(Packet_ChangeBlockData)) { return false; }
		Packet_ChangeBlockData packet = {};
		std::memcpy(&packet, data, sizeof(packet));
		return packet.blockDataHeader.dataSize == size - sizeof(packet);
	}

	bool validateClientOverwriteItem(const char *data, std::size_t size)
	{
		if (size < sizeof(Packet_ClientOverWriteItem)) { return false; }
		Packet_ClientOverWriteItem packet = {};
		std::memcpy(&packet, data, sizeof(packet));
		return packet.metadataSize == size - sizeof(packet);
	}

	bool validateClientBlockDataRecord(const char *data, std::size_t size)
	{
		if (size < sizeof(Packet_ClientChangeBlockData)) { return false; }
		Packet_ClientChangeBlockData packet = {};
		std::memcpy(&packet, data, sizeof(packet));
		return packet.blockDataHeader.dataSize == size - sizeof(packet);
	}

	bool validateClientActionResync(const char *data, std::size_t size)
	{
		if (size < sizeof(Packet_ClientRequestActionResync)) { return false; }
		Packet_ClientRequestActionResync packet = {};
		std::memcpy(&packet, data, sizeof(packet));
		constexpr std::size_t maximumPositions = 64u;
		if (packet.oldestEvent.counter == 0 || packet.oldestEvent.revision == 0 ||
			packet.blockPositionCount > maximumPositions)
		{
			return false;
		}
		const std::size_t expectedSize = sizeof(packet) +
			static_cast<std::size_t>(packet.blockPositionCount) * sizeof(Packet_BlockPositionWire);
		return size == expectedSize;
	}
}

bool getServerPacketPayloadRule(std::uint32_t header, PacketPayloadRule &rule)
{
	switch (header)
	{
		case headerRecieveChunk: rule = exactRule<Packet_RecieveChunk>(); return true;
		case headerRecieveEntireBlockDataForChunk:
		case headerRecieveUpdatesBlockDataForChunk:
			rule = {sizeof(BlockDataHeader), 8u * 1024u * 1024u, 1}; return true;
		case headerChangeBlockData:
			rule = {sizeof(Packet_ChangeBlockData), 1024u * 1024u, 1}; return true;
		case headerPlaceBlock: rule = exactRule<Packet_PlaceBlocks>(); return true;
		case headerPlaceBlocks:
			rule = {sizeof(Packet_PlaceBlocks), 4u * 1024u * 1024u,
				sizeof(Packet_PlaceBlocks)}; return true;
		case headerTrainingDummyGotAttacked:
			rule = exactRule<Packet_TrainingDummyGotAttacked>(); return true;
		case headerValidateEvent: rule = exactRule<Packet_ValidateEvent>(); return true;
		case headerValidateEventAndChangeID:
			rule = exactRule<Packet_ValidateEventAndChangeId>(); return true;
		case headerInValidateEvent: rule = exactRule<Packet_InValidateEvent>(); return true;
		case headerClientRecieveOtherPlayerPosition:
			rule = exactRule<Packet_ClientRecieveOtherPlayerPosition>(); return true;
		case headerUpdateGenericEntity:
			rule = {sizeof(Packet_UpdateGenericEntity) + 1u, 64u * 1024u, 1}; return true;
		case headerClientUpdateTimer: rule = exactRule<Packet_ClientUpdateTimer>(); return true;
		case headerRemoveEntity: rule = exactRule<Packet_RemoveEntity>(); return true;
		case headerKillEntity: rule = exactRule<Packet_KillEntity>(); return true;
		case headerDisconnectOtherPlayer:
			rule = exactRule<Packet_DisconectOtherPlayer>(); return true;
		case headerClientRecieveAllInventory:
			rule = {1u, 1024u * 1024u, 1}; return true;
		case headerRecieveExitBlockInteraction:
			rule = exactRule<Packet_RecieveExitBlockInteraction>(); return true;
		case headerUpdateOwnOtherPlayerSettings:
			rule = exactRule<Packet_UpdateOwnOtherPlayerSettings>(); return true;
		case headerSendPlayerSkin:
			rule = {4u * PLAYER_SKIN_SIZE * PLAYER_SKIN_SIZE,
				4u * PLAYER_SKIN_SIZE * PLAYER_SKIN_SIZE, 1}; return true;
		case headerUpdateLife:
		case headerRecieveDamage:
		case headerRecieveLife:
			rule = exactRule<Packet_UpdateLife>(); return true;
		case headerUpdateSurvivalStats:
			rule = exactRule<Packet_UpdateSurvivalStats>(); return true;
		case headerUpdateSiegeStatus:
			rule = exactRule<Packet_UpdateSiegeStatus>(); return true;
		case headerUpdateWorldTime: rule = exactRule<Packet_UpdateWorldTime>(); return true;
		case headerUpdateWorldDifficulty:
			rule = exactRule<Packet_UpdateWorldDifficulty>(); return true;
		case headerRespawnPlayer: rule = exactRule<Packet_RespawnPlayer>(); return true;
		case headerUpdateEffects: rule = exactRule<Packet_UpdateEffects>(); return true;
		case headerSendChat: rule = {2u, 260u, 1}; return true;
		case headerUpdateGuideProgress: rule = exactRule<GuideProgress>(); return true;
		default: return false;
	}
}

bool validateServerPacketPayload(std::uint32_t header, const char *data, std::size_t size)
{
	PacketPayloadRule rule;
	if (!getServerPacketPayloadRule(header, rule)) { return false; }
	if (!data || size < rule.minimumSize || size > rule.maximumSize) { return false; }
	if (rule.sizeMultiple == 0 || size % rule.sizeMultiple != 0) { return false; }
	if (header == headerRecieveEntireBlockDataForChunk ||
		header == headerRecieveUpdatesBlockDataForChunk)
	{
		return validateBlockDataRecords(data, size);
	}
	if (header == headerChangeBlockData)
	{
		return validateSingleBlockDataRecord(data, size);
	}
	return true;
}

std::size_t maximumDecompressedServerPayload(std::uint32_t header)
{
	PacketPayloadRule rule;
	return getServerPacketPayloadRule(header, rule) ? rule.maximumSize : 0u;
}

bool getClientPacketPayloadRule(std::uint32_t header, bool compressed,
	PacketPayloadRule &rule)
{
	const std::size_t rawSkinSize = 4u * PLAYER_SKIN_SIZE * PLAYER_SKIN_SIZE;
	switch (header)
	{
		case headerClientIdentity: rule = exactRule<Packet_ClientIdentity>(); return true;
		case headerClientDroppedChunk: rule = exactRule<Packet_ClientDroppedChunk>(); return true;
		case headerClientDroppedAllChunks: rule = {0u, 0u, 1u}; return true;
		case headerPlaceBlock: rule = exactRule<Packet_ClientPlaceBlock>(); return true;
		case headerPlaceBlockForce: rule = exactRule<Packet_ClientPlaceBlockForce>(); return true;
		case headerBreakBlock: rule = exactRule<Packet_ClientBreakBlock>(); return true;
		case headerSendPlayerData: rule = exactRule<Packer_SendPlayerData>(); return true;
		case headerClientDroppedItem: rule = exactRule<Packet_ClientDroppedItem>(); return true;
		case headerClientMovedItem: rule = exactRule<Packet_ClientMovedItem>(); return true;
		case headerClientCraftedItem: rule = exactRule<Packet_ClientCraftedItem>(); return true;
		case headerClientOverWriteItem:
			rule = {sizeof(Packet_ClientOverWriteItem),
				sizeof(Packet_ClientOverWriteItem) + 65'535u, 1u}; return true;
		case headerClientSwapItems: rule = exactRule<Packet_ClientSwapItems>(); return true;
		case headerClientUsedItem: rule = exactRule<Packet_ClientUsedItem>(); return true;
		case headerClientInteractWithBlock:
			rule = exactRule<Packet_ClientInteractWithBlock>(); return true;
		case headerRecieveExitBlockInteraction:
			rule = exactRule<Packet_RecieveExitBlockInteraction>(); return true;
		case headerSendPlayerSkin:
			rule = compressed ? PacketPayloadRule{1u, rawSkinSize, 1u} :
				PacketPayloadRule{rawSkinSize, rawSkinSize, 1u}; return true;
		case headerAttackEntity: rule = exactRule<Packet_AttackEntity>(); return true;
		case headerClientWantsToRespawn: rule = {0u, 0u, 1u}; return true;
		case headerClientDamageLocally:
			rule = exactRule<Packet_ClientDamageLocally>(); return true;
		case headerClientDamageLocallyAndDied: rule = {0u, 0u, 1u}; return true;
		case headerSendChat: rule = {1u, 260u, 1u}; return true;
		case headerClientChangeBlockData:
			rule = {sizeof(Packet_ClientChangeBlockData),
				sizeof(Packet_ClientChangeBlockData) + 65'535u, 1u}; return true;
		case headerClientRequestActionResync:
			rule = {sizeof(Packet_ClientRequestActionResync),
				sizeof(Packet_ClientRequestActionResync) +
					64u * sizeof(Packet_BlockPositionWire), 1u}; return true;
		default: return false;
	}
}

bool validateClientPacketPayload(std::uint32_t header, const char *data,
	std::size_t size, bool compressed)
{
	PacketPayloadRule rule;
	if (!getClientPacketPayloadRule(header, compressed, rule)) { return false; }
	if (size < rule.minimumSize || size > rule.maximumSize) { return false; }
	if (size != 0u && !data) { return false; }
	if (rule.sizeMultiple == 0u || size % rule.sizeMultiple != 0u) { return false; }
	if (header == headerClientOverWriteItem)
	{
		return validateClientOverwriteItem(data, size);
	}
	if (header == headerClientChangeBlockData)
	{
		return validateClientBlockDataRecord(data, size);
	}
	if (header == headerClientRequestActionResync)
	{
		return validateClientActionResync(data, size);
	}
	return true;
}
