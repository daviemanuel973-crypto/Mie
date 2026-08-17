#include <gameplay/blocks/furnaceBlock.h>

#include <gameplay/furnaceRecipes.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{
	constexpr char FORMAT_MAGIC[4] = {'M', 'I', 'E', 'F'};
	constexpr std::uint16_t FORMAT_VERSION = 1;
	constexpr float MAX_FURNACE_TIME = 3600.f;
	constexpr float NETWORK_SYNC_INTERVAL = 0.20f;

	template<typename T>
	void appendValue(std::vector<unsigned char> &data, const T &value)
	{
		const auto oldSize = data.size();
		data.resize(oldSize + sizeof(T));
		std::memcpy(data.data() + oldSize, &value, sizeof(T));
	}

	template<typename T>
	bool readValue(const unsigned char *data, size_t size, size_t &pointer, T &value)
	{
		if (pointer > size || size - pointer < sizeof(T)) { return false; }
		std::memcpy(&value, data + pointer, sizeof(T));
		pointer += sizeof(T);
		return true;
	}

	std::array<FurnaceStack, FURNACE_INPUT_CAPACITY> getInputStacks(const FurnaceBlock &furnace)
	{
		std::array<FurnaceStack, FURNACE_INPUT_CAPACITY> result = {};
		for (int i = 0; i < FURNACE_INPUT_CAPACITY; ++i)
		{
			result[i] = {furnace.items[i].type, furnace.items[i].counter};
		}
		return result;
	}

	bool outputFits(const FurnaceBlock &furnace, const FurnaceRecipeDefinition &recipe)
	{
		const Item &output = furnace.items[FURNACE_OUTPUT_SLOT];
		if (output.type == 0) { return true; }
		if (output.type != recipe.output.type || !output.metaData.empty()) { return false; }
		Item copy = output;
		return static_cast<unsigned int>(output.counter) + recipe.output.count <= copy.getStackSize();
	}

	void consumeInputs(FurnaceBlock &furnace, const FurnaceRecipeDefinition &recipe)
	{
		bool used[FURNACE_INPUT_CAPACITY] = {};
		for (const auto &needed : recipe.inputs)
		{
			if (needed.type == 0) { break; }
			for (int slot = 0; slot < FURNACE_INPUT_CAPACITY; ++slot)
			{
				const bool typeMatches = needed.anyWoodLog
					? isFurnaceWoodLog(furnace.items[slot].type)
					: furnace.items[slot].type == needed.type;
				if (!used[slot] && typeMatches && furnace.items[slot].counter >= needed.count)
				{
					used[slot] = true;
					furnace.items[slot].counter -= needed.count;
					if (furnace.items[slot].counter == 0) { furnace.items[slot] = {}; }
					break;
				}
			}
		}
	}

	void produceOutput(FurnaceBlock &furnace, const FurnaceRecipeDefinition &recipe)
	{
		Item &output = furnace.items[FURNACE_OUTPUT_SLOT];
		if (output.type == 0) { output = Item(recipe.output.type, recipe.output.count); }
		else { output.counter += recipe.output.count; }
	}
}

size_t FurnaceBlock::formatIntoData(std::vector<unsigned char> &appendTo) const
{
	const size_t start = appendTo.size();
	appendTo.insert(appendTo.end(), FORMAT_MAGIC, FORMAT_MAGIC + sizeof(FORMAT_MAGIC));
	appendValue(appendTo, FORMAT_VERSION);
	appendValue(appendTo, progressSeconds);
	appendValue(appendTo, fuelSecondsRemaining);
	appendValue(appendTo, fuelSecondsTotal);
	appendValue(appendTo, activeRecipe);
	for (const Item &item : items)
	{
		Item copy = item;
		copy.formatIntoData(appendTo);
	}
	return appendTo.size() - start;
}

bool FurnaceBlock::readFromBuffer(const unsigned char *data, size_t size, size_t &outReadSize)
{
	outReadSize = 0;
	if (!data || size < sizeof(FORMAT_MAGIC) + sizeof(FORMAT_VERSION)) { return false; }
	if (std::memcmp(data, FORMAT_MAGIC, sizeof(FORMAT_MAGIC)) != 0) { return false; }

	FurnaceBlock candidate;
	size_t pointer = sizeof(FORMAT_MAGIC);
	std::uint16_t version = 0;
	if (!readValue(data, size, pointer, version) || version != FORMAT_VERSION) { return false; }
	if (!readValue(data, size, pointer, candidate.progressSeconds) ||
		!readValue(data, size, pointer, candidate.fuelSecondsRemaining) ||
		!readValue(data, size, pointer, candidate.fuelSecondsTotal) ||
		!readValue(data, size, pointer, candidate.activeRecipe))
	{
		return false;
	}

	for (Item &item : candidate.items)
	{
		if (pointer >= size) { return false; }
		const int read = item.readFromData(const_cast<unsigned char *>(data + pointer), size - pointer);
		if (read <= 0 || static_cast<size_t>(read) > size - pointer) { return false; }
		pointer += static_cast<size_t>(read);
	}
	if (pointer != size) { return false; }

	if (!candidate.isDataValid()) { return false; }
	candidate.normalize();
	*this = std::move(candidate);
	outReadSize = pointer;
	return true;
}

bool FurnaceBlock::isDataValid() const
{
	if (!std::isfinite(progressSeconds) || !std::isfinite(fuelSecondsRemaining) ||
		!std::isfinite(fuelSecondsTotal))
	{
		return false;
	}
	if (progressSeconds < 0.f || progressSeconds > MAX_FURNACE_TIME ||
		fuelSecondsRemaining < 0.f || fuelSecondsRemaining > MAX_FURNACE_TIME ||
		fuelSecondsTotal < 0.f || fuelSecondsTotal > MAX_FURNACE_TIME)
	{
		return false;
	}
	return activeRecipe == INVALID_RECIPE || activeRecipe < getFurnaceRecipes().size();
}

void FurnaceBlock::normalize()
{
	for (Item &item : items) { item.sanitize(); }
	if (!std::isfinite(progressSeconds)) { progressSeconds = 0.f; }
	if (!std::isfinite(fuelSecondsRemaining)) { fuelSecondsRemaining = 0.f; }
	if (!std::isfinite(fuelSecondsTotal)) { fuelSecondsTotal = 0.f; }
	progressSeconds = std::max(0.f, std::min(MAX_FURNACE_TIME, progressSeconds));
	fuelSecondsRemaining = std::max(0.f, std::min(MAX_FURNACE_TIME, fuelSecondsRemaining));
	fuelSecondsTotal = std::max(0.f, std::min(MAX_FURNACE_TIME, fuelSecondsTotal));
	if (activeRecipe >= getFurnaceRecipes().size())
	{
		activeRecipe = INVALID_RECIPE;
		progressSeconds = 0.f;
	}
	if (fuelSecondsRemaining <= 0.f) { fuelSecondsRemaining = 0.f; }
	if (fuelSecondsTotal < fuelSecondsRemaining) { fuelSecondsTotal = fuelSecondsRemaining; }
	networkSyncAccumulator = 0.f;
}

FurnaceTickResult FurnaceBlock::tick(float deltaTime)
{
	FurnaceTickResult result;
	if (!std::isfinite(deltaTime) || deltaTime <= 0.f) { return result; }
	deltaTime = std::min(deltaTime, 1.f);

	int recipeIndex = findFurnaceRecipe(getInputStacks(*this));
	if (recipeIndex < 0)
	{
		if (activeRecipe != INVALID_RECIPE || progressSeconds != 0.f)
		{
			activeRecipe = INVALID_RECIPE;
			progressSeconds = 0.f;
			result.changed = result.needsNetworkSync = true;
		}
		return result;
	}

	if (activeRecipe != static_cast<std::uint16_t>(recipeIndex))
	{
		activeRecipe = static_cast<std::uint16_t>(recipeIndex);
		progressSeconds = 0.f;
		result.changed = result.needsNetworkSync = true;
	}

	float remainingDelta = deltaTime;
	while (remainingDelta > 0.f && recipeIndex >= 0)
	{
		const auto &recipe = getFurnaceRecipes()[recipeIndex];
		if (!outputFits(*this, recipe)) { break; }

		if (fuelSecondsRemaining <= 0.f)
		{
			Item &fuel = items[FURNACE_FUEL_SLOT];
			const float fuelDuration = getFurnaceFuelSeconds(fuel.type);
			if (fuelDuration <= 0.f || fuel.counter == 0) { break; }
			fuel.counter--;
			if (fuel.counter == 0) { fuel = {}; }
			fuelSecondsRemaining = fuelDuration;
			fuelSecondsTotal = fuelDuration;
			result.changed = result.needsNetworkSync = true;
		}

		const float untilComplete = std::max(0.f, recipe.durationSeconds - progressSeconds);
		const float step = std::min(remainingDelta, std::min(fuelSecondsRemaining, untilComplete));
		if (step <= 0.f) { break; }
		progressSeconds += step;
		fuelSecondsRemaining -= step;
		remainingDelta -= step;
		result.changed = true;

		if (progressSeconds + 0.0001f >= recipe.durationSeconds)
		{
			consumeInputs(*this, recipe);
			produceOutput(*this, recipe);
			progressSeconds = 0.f;
			activeRecipe = INVALID_RECIPE;
			result.needsNetworkSync = true;

			recipeIndex = findFurnaceRecipe(getInputStacks(*this));
			if (recipeIndex >= 0)
			{
				activeRecipe = static_cast<std::uint16_t>(recipeIndex);
			}
		}
	}

	if (result.changed)
	{
		networkSyncAccumulator += deltaTime;
		if (networkSyncAccumulator >= NETWORK_SYNC_INTERVAL)
		{
			networkSyncAccumulator = 0.f;
			result.needsNetworkSync = true;
		}
	}
	return result;
}

float FurnaceBlock::progressFraction() const
{
	if (activeRecipe == INVALID_RECIPE || activeRecipe >= getFurnaceRecipes().size()) { return 0.f; }
	const float duration = getFurnaceRecipes()[activeRecipe].durationSeconds;
	return duration > 0.f ? std::max(0.f, std::min(1.f, progressSeconds / duration)) : 0.f;
}

float FurnaceBlock::fuelFraction() const
{
	return fuelSecondsTotal > 0.f
		? std::max(0.f, std::min(1.f, fuelSecondsRemaining / fuelSecondsTotal))
		: 0.f;
}

bool FurnaceBlock::canPlaceInSlot(const Item &item, int slot) const
{
	if (item.type == 0) { return true; }
	if (slot >= 0 && slot < FURNACE_INPUT_CAPACITY) { return isFurnaceInput(item.type); }
	if (slot == FURNACE_FUEL_SLOT) { return isFurnaceFuel(item.type); }
	return false;
}

bool isFurnaceInventoryIndex(int index)
{
	return index >= PlayerInventory::CHEST_START_INDEX &&
		index < PlayerInventory::CHEST_START_INDEX + FURNACE_CAPACITY;
}

bool canMoveItemToFurnaceIndex(const Item &item, int index)
{
	if (!isFurnaceInventoryIndex(index)) { return true; }
	if (item.type == 0) { return true; }
	const int slot = index - PlayerInventory::CHEST_START_INDEX;
	if (slot < FURNACE_INPUT_CAPACITY) { return isFurnaceInput(item.type); }
	if (slot == FURNACE_FUEL_SLOT) { return isFurnaceFuel(item.type); }
	return false;
}
