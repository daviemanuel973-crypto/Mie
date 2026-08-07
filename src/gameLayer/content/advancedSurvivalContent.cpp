#include <gameLayer/content/advancedSurvivalContent.h>

namespace ourcraft::content
{

std::string_view advancedSurvivalManifestPath()
{
    return "resources/gameData/content/advanced_survival_v02.json";
}

const std::vector<IntegrationHook> &advancedSurvivalIntegrationHooks()
{
    // This table deliberately names the existing systems that must be touched
    // during stabilization. Keeping the contract here prevents features from
    // being implemented as isolated one-off systems later.
    static const std::vector<IntegrationHook> hooks = {
        {IntegrationArea::Farming, "BlockData + server tick + chunk save", "persistent crop state, growth and harvesting"},
        {IntegrationArea::Cooking, "crafting/furnace/cooking-pot", "data-driven recipes, hunger and temporary buffs"},
        {IntegrationArea::BackpackInventory, "Inventory + serialization + multiplayer sync", "expand capacity without renumbering existing slots"},
        {IntegrationArea::BaseProgression, "server world scan + player progression save", "detect shelter/camp/base/fortress and unlock recipes"},
        {IntegrationArea::Workstations, "blocks + crafting UI", "advanced workbench, campfire, anvil and alchemy capabilities"},
        {IntegrationArea::EquipmentUpgrades, "Item metadata + anvil UI + combat/mining", "persistent upgrade modifiers and repair"},
        {IntegrationArea::UniqueTreasures, "structure loot + Item metadata", "rare non-craftable exploration rewards"},
        {IntegrationArea::DiscoveryJournal, "client journal + player save + server discovery events", "first-time biome/structure/enemy/resource discoveries"},
        {IntegrationArea::WorldEvents, "server world clock + spawn manager", "fog, quiet night, blood moon, raids and meteor events"},
        {IntegrationArea::CaveGeneration, "world generator + structures", "depth-weighted cave themes and underground POIs"},
        {IntegrationArea::WorldMap, "visited-chunk save + UI", "fog-of-war map with discovery-gated POI markers"},
        {IntegrationArea::NightDanger, "server spawn manager", "night spawn multipliers and variant probability adjustments"},
    };
    return hooks;
}

} // namespace ourcraft::content
