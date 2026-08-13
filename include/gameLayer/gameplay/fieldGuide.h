#pragma once

#include <array>
#include <cstdint>

struct PlayerInventory;

enum class GuideObjective : std::uint8_t
{
	GatherWood = 0,
	BuildWorkbench,
	BuildFurnace,
	MakeCharcoal,
	EnterBronzeAge,
	Count,
};

struct GuideProgress
{
	std::uint32_t completedMask = 0;
	std::uint32_t rewardedMask = 0;

	void sanitize();
	bool completed(GuideObjective objective) const;
	bool rewarded(GuideObjective objective) const;
};

static_assert(sizeof(GuideProgress) == 8, "v0.7 Field Guide wire/save layout must remain 8 bytes");

struct GuideObjectiveDefinition
{
	GuideObjective objective = GuideObjective::GatherWood;
	const char *title = "";
	const char *description = "";
	const char *rewardLabel = "";
	std::uint16_t rewardType = 0;
	std::uint16_t rewardCount = 0;
};

const std::array<GuideObjectiveDefinition, 5> &getGuideObjectiveDefinitions();
bool completeGuideObjective(GuideProgress &progress, GuideObjective objective);
bool markGuideRewardClaimed(GuideProgress &progress, GuideObjective objective);
bool guideObjectiveForCraftedType(std::uint16_t type, GuideObjective &objective);
bool isGuideWoodLogType(std::uint16_t type);
bool inventoryContainsGuideWoodLog(const PlayerInventory &inventory);

// Client-side mirror of the authoritative server progress. The shipped v0.7
// executable exposes these symbols and synchronizes the 8-byte GuideProgress
// payload over packet header 51.
GuideProgress getClientGuideProgress();
void setClientGuideProgress(GuideProgress progress);
void resetClientGuideProgress();

// Attempts every completed, unclaimed reward transactionally. A reward is only
// marked claimed when its entire stack fits, matching the shipped v0.7 behavior.
bool deliverPendingGuideRewards(GuideProgress &progress, PlayerInventory &inventory);
