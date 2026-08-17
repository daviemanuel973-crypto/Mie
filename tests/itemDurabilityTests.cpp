#include <gameplay/itemDurability.h>
#include <gameplay/items.h>

#include <cmath>
#include <cstdlib>
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
	std::vector<unsigned char> metadata;

	REQUIRE(getMaximumItemDurability(ItemTypes::goldPickaxe) == 96);
	REQUIRE(getMaximumItemDurability(ItemTypes::trainingSword) == 128);
	REQUIRE(getMaximumItemDurability(ItemTypes::copperAxe) == 180);
	REQUIRE(getMaximumItemDurability(ItemTypes::bronzeSword) == 240);
	REQUIRE(getMaximumItemDurability(ItemTypes::silverSpear) == 260);
	REQUIRE(getMaximumItemDurability(ItemTypes::leadShovel) == 280);
	REQUIRE(getMaximumItemDurability(ItemTypes::ironWarHammer) == 360);
	REQUIRE(!itemUsesDurability(ItemTypes::apple));
	for (unsigned short type = ItemTypes::copperPickaxe;
		type <= ItemTypes::goldBattleAxe; ++type)
	{
		REQUIRE(itemUsesDurability(type));
	}
	for (unsigned short type = ItemTypes::bronzePickaxe;
		type <= ItemTypes::bronzeSword; ++type)
	{
		REQUIRE(itemUsesDurability(type));
	}

	// Legacy and freshly crafted tools have no metadata and therefore start full.
	REQUIRE(getRemainingItemDurability(ItemTypes::ironPickaxe, metadata) == 360);
	REQUIRE(std::fabs(getItemDurabilityFraction(ItemTypes::ironPickaxe, metadata) - 1.f) < 0.0001f);
	REQUIRE(consumeItemDurability(ItemTypes::ironPickaxe, metadata) ==
		ItemDurabilityUseResult::damaged);
	REQUIRE(getRemainingItemDurability(ItemTypes::ironPickaxe, metadata) == 359);
	REQUIRE(metadata.size() == 7);

	// v0.9 repairs use the same metadata and cap safely at the material maximum.
	REQUIRE(repairItemDurability(ItemTypes::ironPickaxe, metadata, 20) == 1);
	REQUIRE(getRemainingItemDurability(ItemTypes::ironPickaxe, metadata) == 360);
	REQUIRE(metadata.empty());
	REQUIRE(repairItemDurability(ItemTypes::ironPickaxe, metadata, 20) == 0);
	REQUIRE(repairItemDurability(ItemTypes::apple, metadata, 20) == 0);

	REQUIRE(consumeItemDurability(ItemTypes::bronzeSword, metadata, 100) ==
		ItemDurabilityUseResult::damaged);
	REQUIRE(getRemainingItemDurability(ItemTypes::bronzeSword, metadata) == 140);
	REQUIRE(repairItemDurability(ItemTypes::bronzeSword, metadata, 40) == 40);
	REQUIRE(getRemainingItemDurability(ItemTypes::bronzeSword, metadata) == 180);
	REQUIRE(repairItemDurability(ItemTypes::bronzeSword, metadata, 1000) == 60);
	REQUIRE(getRemainingItemDurability(ItemTypes::bronzeSword, metadata) == 240);
	REQUIRE(metadata.empty());

	// The same canonical payload can be persisted and resumed.
	REQUIRE(consumeItemDurability(ItemTypes::ironPickaxe, metadata) ==
		ItemDurabilityUseResult::damaged);
	auto persistedMetadata = metadata;
	REQUIRE(consumeItemDurability(ItemTypes::ironPickaxe, persistedMetadata, 358) ==
		ItemDurabilityUseResult::damaged);
	REQUIRE(getRemainingItemDurability(ItemTypes::ironPickaxe, persistedMetadata) == 1);
	REQUIRE(consumeItemDurability(ItemTypes::ironPickaxe, persistedMetadata) ==
		ItemDurabilityUseResult::broken);

	// Unknown legacy metadata is sanitized into a compatible full item.
	std::vector<unsigned char> unknownMetadata = {1, 2, 3, 4};
	REQUIRE(sanitizeItemDurabilityMetadata(ItemTypes::copperPickaxe, unknownMetadata));
	REQUIRE(unknownMetadata.empty());
	REQUIRE(getRemainingItemDurability(ItemTypes::copperPickaxe, unknownMetadata) == 180);
	std::vector<unsigned char> oversizedMetadata = {'M', 'I', 'E', 'D', 1, 0xff, 0xff};
	REQUIRE(sanitizeItemDurabilityMetadata(ItemTypes::goldSword, oversizedMetadata));
	REQUIRE(getRemainingItemDurability(ItemTypes::goldSword, oversizedMetadata) == 96);

	// Metadata does not change nondurable items.
	std::vector<unsigned char> unrelatedMetadata = {9, 8, 7};
	REQUIRE(sanitizeItemDurabilityMetadata(ItemTypes::apple, unrelatedMetadata));
	REQUIRE(unrelatedMetadata == std::vector<unsigned char>({9, 8, 7}));
	REQUIRE(consumeItemDurability(ItemTypes::apple, unrelatedMetadata) ==
		ItemDurabilityUseResult::notApplicable);

	// A valid persisted zero is rejected instead of reviving a broken item.
	std::vector<unsigned char> zeroMetadata = {'M', 'I', 'E', 'D', 1, 1, 0};
	REQUIRE(consumeItemDurability(ItemTypes::bronzePickaxe, zeroMetadata) ==
		ItemDurabilityUseResult::broken);
	zeroMetadata = {'M', 'I', 'E', 'D', 1, 0, 0};
	REQUIRE(!sanitizeItemDurabilityMetadata(ItemTypes::bronzePickaxe, zeroMetadata));

	std::cout << "Item durability tests passed.\n";
	return 0;
}
