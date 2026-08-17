#include <gameplay/crafting.h>

#include <iostream>

// Keep this contract focused on the recipe registry and discovery rules. The
// full game supplies these legacy helpers from blocks/items/Field Guide code.
bool isWoodPlank(BlockType type)
{
	return type == BlockTypes::wooden_plank || type == BlockTypes::jungle_planks ||
		type == BlockTypes::sprucePlank || type == BlockTypes::birchPlanks;
}

bool isGuideWoodLogType(std::uint16_t type)
{
	return type == 5 || type == 20 || type == 25 || type == 27 ||
		type == 43 || type == 164 || type == 165 || type == 166;
}

bool areItemsTheSame(Item &a, Item &b)
{
	return a.type == b.type && a.metaData == b.metaData;
}

Item itemCreator(unsigned short type, unsigned short counter)
{
	return Item(type, counter);
}

namespace
{
	int failures = 0;

	void check(bool condition, const char *message)
	{
		if (!condition)
		{
			std::cerr << "FAILED: " << message << '\n';
			++failures;
		}
	}
}

int main()
{
	check(getCraftingRecipeCount() == 103,
		"the shipped recipe registry keeps all 103 stable packet indexes");
	check(!isCraftingRecipeDiscovered(-1, {}), "negative recipe indexes are rejected");
	check(!isCraftingRecipeDiscovered(getCraftingRecipeCount(), {}),
		"out-of-range recipe indexes are rejected");

	RecipeDiscovery discovery;
	check(getDiscoveredCraftingRecipes(discovery).empty(),
		"an empty material history does not spoil recipes");

	discovery.learnType(BlockTypes::woodLog);
	check(isCraftingRecipeDiscovered(0, discovery),
		"finding an oak log reveals its plank recipe");
	check(!isCraftingRecipeDiscovered(1, discovery),
		"an exact spruce-log recipe stays hidden until spruce is found");
	check(!isCraftingRecipeDiscovered(7, discovery),
		"plank recipes stay hidden before the player finds planks");

	discovery.learnType(BlockTypes::birchPlanks);
	check(isCraftingRecipeDiscovered(7, discovery),
		"any discovered plank family reveals the any-wood stick recipe");
	check(isCraftingRecipeDiscovered(8, discovery),
		"any discovered plank family reveals the workbench recipe");

	discovery.learnType(BlockTypes::cobblestone);
	check(isCraftingRecipeDiscovered(9, discovery),
		"finding cobblestone reveals the survival furnace recipe");
	check(std::string(getCraftingRecipeStationName(getRecepieFromIndexUnsafe(9))) == "WORKBENCH",
		"the recipe book exposes the correct required station");

	discovery.learnType(ItemTypes::cloth);
	check(!isCraftingRecipeDiscovered(13, discovery),
		"multi-ingredient recipes wait until every ingredient type was found");
	discovery.learnType(ItemTypes::wheat);
	check(isCraftingRecipeDiscovered(13, discovery),
		"multi-ingredient recipes unlock permanently after all types are known");

	const auto discovered = getDiscoveredCraftingRecipes(discovery);
	check(!discovered.empty() && discovered.front().index == 0,
		"the recipe book preserves stable registry order for pagination");

	if (failures == 0) { std::cout << "All recipe book tests passed\n"; }
	return failures == 0 ? 0 : 1;
}
