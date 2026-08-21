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
