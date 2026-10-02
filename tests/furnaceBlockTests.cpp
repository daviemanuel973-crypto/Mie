#include <gameplay/blocks/furnaceBlock.h>

#include <blocks.h>
#include <gameplay/items.h>

#include <cstring>
#include <iostream>
#include <limits>

#include "processingItemAdapters.h"

namespace
{
	int failures = 0;
	void check(bool condition, const char *message)
	{
		if (!condition)
		{
			std::cerr << "FAIL: " << message << '\n';
			++failures;
		}
	}
}

int main()
{
	FurnaceBlock copper;
	copper.items[0] = Item(BlockTypes::copperOre, 2);
	copper.items[FURNACE_FUEL_SLOT] = Item(ItemTypes::charcoal, 1);
	auto first = copper.tick(0.1f);
	check(first.changed && first.needsNetworkSync, "starting consumes one fuel and requests sync");
	check(copper.items[FURNACE_FUEL_SLOT].type == 0 && copper.fuelSecondsRemaining > 15.f,
		"charcoal starts a sixteen-second burn");
	for (int i = 0; i < 8; ++i) { copper.tick(1.f); }
	check(copper.items[FURNACE_OUTPUT_SLOT].type == ItemTypes::copperIngot,
		"copper completes after eight seconds");
	check(copper.items[0].type == 0, "completed processing consumes the ore");

	FurnaceBlock blocked;
	blocked.items[0] = Item(BlockTypes::ironOre, 3);
	blocked.items[FURNACE_FUEL_SLOT] = Item(BlockTypes::wooden_plank, 1);
	blocked.items[FURNACE_OUTPUT_SLOT] = Item(ItemTypes::goldIngot, 1);
	blocked.tick(1.f);
	check(blocked.items[FURNACE_FUEL_SLOT].counter == 1 && blocked.progressSeconds == 0.f,
		"a blocked output neither consumes fuel nor advances progress");

	FurnaceBlock bronze;
	bronze.items[0] = Item(ItemTypes::tinIngot, 1);
	bronze.items[1] = Item(ItemTypes::charcoal, 1);
	bronze.items[2] = Item(ItemTypes::copperIngot, 3);
	bronze.items[FURNACE_FUEL_SLOT] = Item(BlockTypes::woodLog, 2);
	for (int i = 0; i < 11; ++i) { bronze.tick(1.f); }
	check(bronze.items[FURNACE_OUTPUT_SLOT].type == ItemTypes::bronzeIngot &&
		bronze.items[FURNACE_OUTPUT_SLOT].counter == 4,
		"bronze supports all three input cells and produces four ingots");

	check(!canMoveItemToFurnaceIndex(Item(ItemTypes::copperIngot),
		PlayerInventory::CHEST_START_INDEX + FURNACE_OUTPUT_SLOT),
		"players cannot insert items into the output cell");
	check(canMoveItemToFurnaceIndex(Item(ItemTypes::charcoal),
		PlayerInventory::CHEST_START_INDEX + FURNACE_FUEL_SLOT),
		"fuel can enter only through the fuel policy");

	std::vector<unsigned char> payload;
	bronze.formatIntoData(payload);
	FurnaceBlock restored;
	size_t read = 0;
	check(restored.readFromBuffer(payload.data(), payload.size(), read) && read == payload.size(),
		"versioned furnace state round-trips exactly");
	check(restored.items[FURNACE_OUTPUT_SLOT].type == ItemTypes::bronzeIngot &&
		restored.items[FURNACE_OUTPUT_SLOT].counter == 4,
		"output survives persistence and network serialization");

	auto trailing = payload;
	trailing.push_back(0xff);
	FurnaceBlock rejected;
	read = 0;
	check(!rejected.readFromBuffer(trailing.data(), trailing.size(), read),
		"trailing bytes are rejected instead of desynchronizing the next block record");
	auto nonFinite = payload;
	const float nan = std::numeric_limits<float>::quiet_NaN();
	std::memcpy(nonFinite.data() + 4 + sizeof(std::uint16_t), &nan, sizeof(nan));
	read = 0;
	check(!rejected.readFromBuffer(nonFinite.data(), nonFinite.size(), read),
		"non-finite progress is rejected during world loading");

	if (failures == 0) { std::cout << "All furnace block tests passed\n"; }
	return failures == 0 ? 0 : 1;
}
