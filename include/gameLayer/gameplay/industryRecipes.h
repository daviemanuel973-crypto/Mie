#pragma once
#include <gameplay/blocks/furnaceBlock.h>
#include <gameplay/furnaceRecipes.h>

struct IndustryProcess
{
	FurnaceRecipeDefinition recipe;
	std::uint16_t id = FurnaceBlock::INVALID_RECIPE;
};

bool isElectricConsumer(std::uint16_t type);
std::uint32_t industryEnergyCapacity(std::uint16_t type);
IndustryProcess findIndustryProcess(const FurnaceBlock &machine);
bool industryOutputFits(const FurnaceBlock &machine, const IndustryProcess &process);
bool industrySlotAccepts(const FurnaceBlock &machine, const Item &item, int slot);
FurnaceTickResult tickIndustryMachine(FurnaceBlock &machine, float seconds);
float industryProgressFraction(const FurnaceBlock &machine);
const char *industryTitle(std::uint16_t type);
