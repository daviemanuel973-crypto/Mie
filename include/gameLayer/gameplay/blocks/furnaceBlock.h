#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <gameplay/items.h>

constexpr int FURNACE_INPUT_CAPACITY = 3;
constexpr int FURNACE_FUEL_SLOT = 3;
constexpr int FURNACE_OUTPUT_SLOT = 4;
constexpr int FURNACE_CAPACITY = 5;

struct FurnaceTickResult
{
	bool changed = false;
	bool needsNetworkSync = false;
};

struct FurnaceBlock
{
	static constexpr std::uint16_t INVALID_RECIPE = 0xffff;

	Item items[FURNACE_CAPACITY] = {};
	float progressSeconds = 0.f;
	float fuelSecondsRemaining = 0.f;
	float fuelSecondsTotal = 0.f;
	std::uint16_t activeRecipe = INVALID_RECIPE;

	// Runtime-only throttle; deliberately omitted from save/network payloads.
	float networkSyncAccumulator = 0.f;

	size_t formatIntoData(std::vector<unsigned char> &appendTo) const;
	bool readFromBuffer(const unsigned char *data, size_t size, size_t &outReadSize);
	bool isDataValid() const;
	void normalize();

	FurnaceTickResult tick(float deltaTime);
	float progressFraction() const;
	float fuelFraction() const;
	bool canPlaceInSlot(const Item &item, int slot) const;
};

bool isFurnaceInventoryIndex(int index);
bool canMoveItemToFurnaceIndex(const Item &item, int index);
