#include <gameplay/fieldGuide.h>

#include <gameplay/items.h>
#include <utility>

namespace
{
	constexpr std::uint32_t validGuideMask = (1u << static_cast<unsigned int>(GuideObjective::Count)) - 1u;

	constexpr std::uint32_t objectiveBit(GuideObjective objective)
	{
		return 1u << static_cast<unsigned int>(objective);
	}
}

void GuideProgress::sanitize()
{
	completedMask &= validGuideMask;
	rewardedMask &= completedMask;
}

bool GuideProgress::completed(GuideObjective objective) const
{
	return (completedMask & objectiveBit(objective)) != 0;
}

bool GuideProgress::rewarded(GuideObjective objective) const
{
	return (rewardedMask & objectiveBit(objective)) != 0;
}

const std::array<GuideObjectiveDefinition, 5> &getGuideObjectiveDefinitions()
{
	static const std::array<GuideObjectiveDefinition, 5> definitions = {{
		{GuideObjective::GatherWood, "Gather Wood", "Collect any tree log.", "Copper Dagger", 2085, 1},
		{GuideObjective::BuildWorkbench, "Build a Workbench", "Craft a workbench.", "4 Cloth", 2049, 4},
		{GuideObjective::BuildFurnace, "Build a Furnace", "Craft a furnace at a workbench.", "4 Charcoal", 2185, 4},
		{GuideObjective::MakeCharcoal, "Make Charcoal", "Fire a log in a furnace.", "2 Cassiterite", 2186, 2},
		{GuideObjective::EnterBronzeAge, "Enter the Bronze Age", "Alloy the first bronze ingots.", "Bronze Sword", 2192, 1},
	}};
	return definitions;
}

bool completeGuideObjective(GuideProgress &progress, GuideObjective objective)
{
	progress.sanitize();
	const std::uint32_t bit = objectiveBit(objective);
	if ((progress.completedMask & bit) != 0) { return false; }
	progress.completedMask |= bit;
	return true;
}

bool markGuideRewardClaimed(GuideProgress &progress, GuideObjective objective)
{
	progress.sanitize();
	const std::uint32_t bit = objectiveBit(objective);
	if ((progress.completedMask & bit) == 0 || (progress.rewardedMask & bit) != 0)
	{
		return false;
	}
	progress.rewardedMask |= bit;
	return true;
}

bool guideObjectiveForCraftedType(std::uint16_t type, GuideObjective &objective)
{
	switch (type)
	{
		case 144: objective = GuideObjective::BuildWorkbench; return true;
		case 204: objective = GuideObjective::BuildFurnace; return true;
		case ItemTypes::charcoal: objective = GuideObjective::MakeCharcoal; return true;
		case ItemTypes::bronzeIngot: objective = GuideObjective::EnterBronzeAge; return true;
		default: return false;
	}
}

bool isGuideWoodLogType(std::uint16_t type)
{
	switch (type)
	{
		case 5:
		case 20:
		case 25:
		case 27:
		case 43:
		case 164:
		case 165:
		case 166:
			return true;
		default:
			return false;
	}
}

bool inventoryContainsGuideWoodLog(const PlayerInventory &inventory)
{
	for (const Item &item : inventory.items)
	{
		if (item.counter > 0 && isGuideWoodLogType(item.type)) { return true; }
	}
	return false;
}

bool deliverPendingGuideRewards(GuideProgress &progress, PlayerInventory &inventory)
{
	progress.sanitize();
	bool changed = false;
	for (const GuideObjectiveDefinition &definition : getGuideObjectiveDefinitions())
	{
		if (!progress.completed(definition.objective) || progress.rewarded(definition.objective))
		{
			continue;
		}

		PlayerInventory candidate = inventory;
		const Item reward = itemCreator(definition.rewardType, definition.rewardCount);
		if (candidate.tryPickupItem(reward) != definition.rewardCount)
		{
			continue;
		}

		inventory = std::move(candidate);
		markGuideRewardClaimed(progress, definition.objective);
		changed = true;
	}
	return changed;
}
