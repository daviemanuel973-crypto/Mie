#include <gameplay/blocks/furnaceBlock.h>

#include <blocks.h>
#include <gameplay/items.h>

#include <cstring>
#include <iostream>
#include <limits>

// Focused Item adapters keep this unit test independent from rendering/audio
// dependencies while exercising the exact FurnaceBlock state machine and format.
std::size_t Item::formatIntoData(std::vector<unsigned char> &data)
{
	const size_t start = data.size();
	const std::uint32_t metadataSize = static_cast<std::uint32_t>(metaData.size());
	data.resize(start + sizeof(type) + sizeof(counter) + sizeof(metadataSize) + metadataSize);
	size_t pointer = start;
	std::memcpy(data.data() + pointer, &type, sizeof(type)); pointer += sizeof(type);
	std::memcpy(data.data() + pointer, &counter, sizeof(counter)); pointer += sizeof(counter);
	std::memcpy(data.data() + pointer, &metadataSize, sizeof(metadataSize)); pointer += sizeof(metadataSize);
	if (metadataSize) { std::memcpy(data.data() + pointer, metaData.data(), metadataSize); }
	return data.size() - start;
}

int Item::readFromData(void *input, size_t size)
{
	if (!input || size < sizeof(type) + sizeof(counter) + sizeof(std::uint32_t)) { return -1; }
	const auto *data = static_cast<unsigned char *>(input);
	size_t pointer = 0;
	std::uint32_t metadataSize = 0;
	std::memcpy(&type, data + pointer, sizeof(type)); pointer += sizeof(type);
	std::memcpy(&counter, data + pointer, sizeof(counter)); pointer += sizeof(counter);
	std::memcpy(&metadataSize, data + pointer, sizeof(metadataSize)); pointer += sizeof(metadataSize);
	if (metadataSize > size - pointer) { return -1; }
	metaData.assign(data + pointer, data + pointer + metadataSize);
	pointer += metadataSize;
	return static_cast<int>(pointer);
}

void Item::sanitize()
{
	if (type == 0 || counter == 0) { *this = {}; }
	else if (counter > getStackSize()) { counter = getStackSize(); }
}

unsigned short Item::getStackSize() { return 999; }

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
