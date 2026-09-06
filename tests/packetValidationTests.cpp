#include <gameplay/fieldGuide.h>
#include <gameplay/fieldGuideProtocol.h>
#include <multyPlayer/packet.h>
#include <multyPlayer/packetValidation.h>
#include <multyPlayer/serverActionValidation.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

namespace
{
	#define REQUIRE(condition) do { if (!(condition)) { \
		std::cerr << "Requirement failed at line " << __LINE__ << ": " #condition "\n"; \
		std::exit(1); } } while (false)
}

int main()
{
	REQUIRE(headerUpdateGuideProgress != headerUpdateWorldDifficulty);
	REQUIRE(headerUpdateGuideProgress == 52);
	REQUIRE(MULTIPLAYER_PROTOCOL_VERSION == 4);

	REQUIRE(mie::serverValidation::isAuthoritativeItemSlotUsable(42, 1, 42));
	REQUIRE(!mie::serverValidation::isAuthoritativeItemSlotUsable(42, 0, 42));
	REQUIRE(!mie::serverValidation::isAuthoritativeItemSlotUsable(42, 1, 41));
	REQUIRE(mie::serverValidation::itemUseRemainsAllowedAfterAction(true, true));
	REQUIRE(!mie::serverValidation::itemUseRemainsAllowedAfterAction(true, false));
	REQUIRE(!mie::serverValidation::itemUseRemainsAllowedAfterAction(false, true));

	Packet_PlaceBlocks oneBlock = {};
	REQUIRE(validateServerPacketPayload(headerPlaceBlock,
		reinterpret_cast<const char *>(&oneBlock), sizeof(oneBlock)));
	REQUIRE(!validateServerPacketPayload(headerPlaceBlock,
		reinterpret_cast<const char *>(&oneBlock), sizeof(oneBlock) - 1));

	std::vector<Packet_PlaceBlocks> blocks(3);
	REQUIRE(validateServerPacketPayload(headerPlaceBlocks,
		reinterpret_cast<const char *>(blocks.data()), blocks.size() * sizeof(blocks.front())));
	REQUIRE(!validateServerPacketPayload(headerPlaceBlocks,
		reinterpret_cast<const char *>(blocks.data()), blocks.size() * sizeof(blocks.front()) - 1));

	Packet_ValidateEvent event = {};
	REQUIRE(validateServerPacketPayload(headerValidateEvent,
		reinterpret_cast<const char *>(&event), sizeof(event)));
	REQUIRE(!validateServerPacketPayload(headerValidateEvent, nullptr, sizeof(event)));

	std::vector<char> skin(4u * PLAYER_SKIN_SIZE * PLAYER_SKIN_SIZE);
	REQUIRE(validateServerPacketPayload(headerSendPlayerSkin, skin.data(), skin.size()));
	REQUIRE(!validateServerPacketPayload(headerSendPlayerSkin, skin.data(), skin.size() - 1));

	std::vector<char> blockData(sizeof(BlockDataHeader) + 3u);
	BlockDataHeader blockHeader = {};
	blockHeader.dataSize = 3;
	std::memcpy(blockData.data(), &blockHeader, sizeof(blockHeader));
	REQUIRE(validateServerPacketPayload(headerRecieveEntireBlockDataForChunk,
		blockData.data(), blockData.size()));
	blockHeader.dataSize = 4;
	std::memcpy(blockData.data(), &blockHeader, sizeof(blockHeader));
	REQUIRE(!validateServerPacketPayload(headerRecieveEntireBlockDataForChunk,
		blockData.data(), blockData.size()));

	std::vector<char> changedBlock(sizeof(Packet_ChangeBlockData) + 2u);
	Packet_ChangeBlockData change = {};
	change.blockDataHeader.dataSize = 2;
	std::memcpy(changedBlock.data(), &change, sizeof(change));
	REQUIRE(validateServerPacketPayload(headerChangeBlockData,
		changedBlock.data(), changedBlock.size()));
	change.blockDataHeader.dataSize = 1;
	std::memcpy(changedBlock.data(), &change, sizeof(change));
	REQUIRE(!validateServerPacketPayload(headerChangeBlockData,
		changedBlock.data(), changedBlock.size()));

	GuideProgress guide = {};
	REQUIRE(validateServerPacketPayload(headerUpdateGuideProgress,
		reinterpret_cast<const char *>(&guide), sizeof(guide)));
	REQUIRE(!validateServerPacketPayload(999999u,
		reinterpret_cast<const char *>(&guide), sizeof(guide)));
	REQUIRE(maximumDecompressedServerPayload(headerRecieveEntireBlockDataForChunk) ==
		8u * 1024u * 1024u);
	REQUIRE(maximumDecompressedServerPayload(999999u) == 0u);

	Packet_ClientDroppedItem droppedItem = {};
	REQUIRE(validateClientPacketPayload(headerClientDroppedItem,
		reinterpret_cast<const char *>(&droppedItem), sizeof(droppedItem)));
	REQUIRE(!validateClientPacketPayload(headerClientDroppedItem,
		reinterpret_cast<const char *>(&droppedItem), sizeof(droppedItem) - 1));
	REQUIRE(validateClientPacketPayload(headerClientDroppedAllChunks, nullptr, 0));
	REQUIRE(!validateClientPacketPayload(headerClientDroppedAllChunks,
		reinterpret_cast<const char *>(&droppedItem), 1));

	std::vector<char> overwrittenItem(sizeof(Packet_ClientOverWriteItem) + 3u);
	Packet_ClientOverWriteItem overwrite = {};
	overwrite.metadataSize = 3;
	std::memcpy(overwrittenItem.data(), &overwrite, sizeof(overwrite));
	REQUIRE(validateClientPacketPayload(headerClientOverWriteItem,
		overwrittenItem.data(), overwrittenItem.size()));
	overwrite.metadataSize = 4;
	std::memcpy(overwrittenItem.data(), &overwrite, sizeof(overwrite));
	REQUIRE(!validateClientPacketPayload(headerClientOverWriteItem,
		overwrittenItem.data(), overwrittenItem.size()));

	std::vector<char> clientBlockData(sizeof(Packet_ClientChangeBlockData) + 2u);
	Packet_ClientChangeBlockData clientChange = {};
	clientChange.blockDataHeader.dataSize = 2;
	std::memcpy(clientBlockData.data(), &clientChange, sizeof(clientChange));
	REQUIRE(validateClientPacketPayload(headerClientChangeBlockData,
		clientBlockData.data(), clientBlockData.size()));
	clientChange.blockDataHeader.dataSize = 1;
	std::memcpy(clientBlockData.data(), &clientChange, sizeof(clientChange));
	REQUIRE(!validateClientPacketPayload(headerClientChangeBlockData,
		clientBlockData.data(), clientBlockData.size()));

	REQUIRE(validateClientPacketPayload(headerSendPlayerSkin, skin.data(), skin.size(), false));
	REQUIRE(!validateClientPacketPayload(headerSendPlayerSkin, skin.data(), skin.size() - 1, false));
	REQUIRE(validateClientPacketPayload(headerSendPlayerSkin, skin.data(), 16, true));
	REQUIRE(!validateClientPacketPayload(999999u,
		reinterpret_cast<const char *>(&droppedItem), sizeof(droppedItem)));

	std::vector<char> resync(sizeof(Packet_ClientRequestActionResync) +
		2u * sizeof(Packet_BlockPositionWire));
	Packet_ClientRequestActionResync resyncHeader = {};
	resyncHeader.oldestEvent = {1, 1};
	resyncHeader.blockPositionCount = 2;
	std::memcpy(resync.data(), &resyncHeader, sizeof(resyncHeader));
	REQUIRE(validateClientPacketPayload(headerClientRequestActionResync,
		resync.data(), resync.size()));
	resyncHeader.blockPositionCount = 3;
	std::memcpy(resync.data(), &resyncHeader, sizeof(resyncHeader));
	REQUIRE(!validateClientPacketPayload(headerClientRequestActionResync,
		resync.data(), resync.size()));

	#ifndef MIE_SKIP_COMPRESSION_TESTS
	std::vector<char> repetitivePayload(256u * 1024u, 0);
	size_t compressedSize = 0;
	char *compressed = static_cast<char *>(compressDataForce(repetitivePayload.data(),
		repetitivePayload.size(), compressedSize));
	REQUIRE(compressed != nullptr && compressedSize > 0);
	size_t originalSize = 123;
	REQUIRE(unCompressDataBounded(compressed, compressedSize, originalSize,
		repetitivePayload.size() - 1) == nullptr);
	REQUIRE(originalSize == 0);
	char *restored = static_cast<char *>(unCompressDataBounded(compressed, compressedSize,
		originalSize, repetitivePayload.size()));
	REQUIRE(restored != nullptr && originalSize == repetitivePayload.size());
	REQUIRE(std::memcmp(restored, repetitivePayload.data(), originalSize) == 0);
	delete[] restored;
	delete[] compressed;
	const char invalidFrame[] = "not-a-zstd-frame";
	REQUIRE(unCompressDataBounded(invalidFrame, sizeof(invalidFrame), originalSize, 1024) == nullptr);
	REQUIRE(originalSize == 0);
	#endif

	std::cout << "Packet validation tests passed.\n";
	return 0;
}
