#pragma once

// WIP content-integration contract for the advanced Survival/Exploration update.
// This header is intentionally not wired into the shipping build yet. It lets us
// define stable content keys and integration boundaries before assigning engine IDs.

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ourcraft::content
{

enum class IntegrationArea : std::uint8_t
{
    Farming,
    Cooking,
    BackpackInventory,
    BaseProgression,
    Workstations,
    EquipmentUpgrades,
    UniqueTreasures,
    DiscoveryJournal,
    WorldEvents,
    CaveGeneration,
    WorldMap,
    NightDanger,
};

struct ContentKey
{
    std::string value;
};

struct ItemStackKey
{
    ContentKey item;
    std::uint16_t amount = 1;
};

struct CropDefinition
{
    ContentKey id;
    ContentKey seed;
    ContentKey produce;
    std::uint8_t growthStages = 1;
    float growthSeconds = 0.f;
    float waterGrowthMultiplier = 1.f;
    std::vector<std::string> preferredBiomes;
    bool rare = false;
};

struct CookingRecipeDefinition
{
    ContentKey id;
    ContentKey station;
    std::vector<ItemStackKey> inputs;
    ItemStackKey output;
    std::uint8_t hungerRestore = 0;
    ContentKey optionalBuff;
    float buffSeconds = 0.f;
};

struct BackpackDefinition
{
    ContentKey id;
    std::uint8_t tier = 0;
    std::uint8_t extraSlots = 0;
    std::vector<ItemStackKey> recipe;
};

struct BaseTierDefinition
{
    ContentKey id;
    std::uint8_t rank = 0;
    std::uint16_t minimumEnclosedBlocks = 0;
    std::uint8_t minimumStorageBlocks = 0;
    std::vector<ContentKey> requiredWorkstations;
    std::vector<ContentKey> unlocks;
};

struct WorkstationDefinition
{
    ContentKey id;
    std::uint8_t tier = 0;
    std::vector<std::string> functions;
};

struct EquipmentUpgradeDefinition
{
    ContentKey id;
    ContentKey station;
    std::uint8_t maxLevel = 1;
    float damagePercentPerLevel = 0.f;
    float durabilityPercentPerLevel = 0.f;
    float attackSpeedPercentPerLevel = 0.f;
    std::vector<ContentKey> costs;
};

struct TreasureDefinition
{
    ContentKey id;
    std::string rarity;
    std::vector<ContentKey> sources;
};

struct DiscoveryDefinition
{
    ContentKey id;
    std::string category;
    std::string trigger;
    std::uint16_t rewardXp = 0;
};

struct WorldEventDefinition
{
    ContentKey id;
    std::uint16_t weight = 0;
    std::uint16_t minimumDay = 0;
    float minimumDurationSeconds = 0.f;
    float maximumDurationSeconds = 0.f;
};

struct CaveThemeDefinition
{
    ContentKey id;
    std::uint8_t weight = 0;
    std::uint16_t minimumDepth = 0;
    std::uint16_t maximumDepth = 0;
    std::vector<std::string> features;
};

struct AdvancedSurvivalRegistry
{
    std::vector<CropDefinition> crops;
    std::vector<CookingRecipeDefinition> cookingRecipes;
    std::vector<BackpackDefinition> backpacks;
    std::vector<BaseTierDefinition> baseTiers;
    std::vector<WorkstationDefinition> workstations;
    std::vector<EquipmentUpgradeDefinition> equipmentUpgrades;
    std::vector<TreasureDefinition> treasures;
    std::vector<DiscoveryDefinition> discoveries;
    std::vector<WorldEventDefinition> worldEvents;
    std::vector<CaveThemeDefinition> caveThemes;
};

struct IntegrationHook
{
    IntegrationArea area;
    std::string_view target;
    std::string_view responsibility;
};

// The manifest is the source of truth during the WIP phase. Engine enum/packet IDs
// are assigned only in the stabilization branch after asset/save/network review.
std::string_view advancedSurvivalManifestPath();
const std::vector<IntegrationHook> &advancedSurvivalIntegrationHooks();

} // namespace ourcraft::content
