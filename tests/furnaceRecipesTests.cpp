#include <gameplay/furnaceRecipes.h>

#include <blocks.h>
#include <gameplay/items.h>

#include <cmath>
#include <iostream>

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
	const auto &recipes = getFurnaceRecipes();
	check(recipes.size() == 8, "the shipped eight furnace recipes remain registered");
	check(recipes[0].inputs[0].type == BlockTypes::copperOre &&
		recipes[0].inputs[0].count == 2 && recipes[0].output.type == ItemTypes::copperIngot,
		"copper keeps the v0.7 contract");
	check(recipes[5].inputs[0].anyWoodLog && recipes[5].output.type == ItemTypes::charcoal &&
		recipes[5].output.count == 2, "charcoal accepts the complete wood-log family");
	check(recipes[7].inputs[0].count == 3 && recipes[7].inputs[1].count == 1 &&
		recipes[7].inputs[2].type == ItemTypes::charcoal && recipes[7].output.count == 4,
		"bronze keeps all three ingredients and its four-ingot output");

	std::array<FurnaceStack, 3> copper = {{{BlockTypes::copperOre, 2}, {}, {}}};
	check(findFurnaceRecipe(copper) == 0, "two copper ore start the copper recipe");
	copper[0].count = 1;
	check(findFurnaceRecipe(copper) == -1, "an incomplete input does not start processing");

	std::array<FurnaceStack, 3> charcoal = {{{BlockTypes::birch_log, 1}, {}, {}}};
	check(findFurnaceRecipe(charcoal) == 5, "birch logs match the any-wood charcoal recipe");

	std::array<FurnaceStack, 3> bronze = {{{ItemTypes::charcoal, 1},
		{ItemTypes::copperIngot, 3}, {ItemTypes::tinIngot, 1}}};
	check(findFurnaceRecipe(bronze) == 7, "multi-input recipes are independent of slot order");

	check(isFurnaceFuel(ItemTypes::charcoal) &&
		std::fabs(getFurnaceFuelSeconds(ItemTypes::charcoal) - 16.f) < 0.001f,
		"charcoal is the longest basic fuel");
	check(isFurnaceFuel(BlockTypes::sprucePlank) && isFurnaceFuel(ItemTypes::stick),
		"wood planks and sticks are valid fallback fuels");
	check(!isFurnaceFuel(ItemTypes::copperIngot), "metal cannot be inserted as fuel");
	check(isFurnaceInput(BlockTypes::goldOre) && isFurnaceInput(ItemTypes::tinConcentrate),
		"all progression inputs are accepted");
	check(!isFurnaceInput(ItemTypes::fieldGuide), "unrelated items are rejected from input cells");

	if (failures == 0) { std::cout << "All furnace recipe tests passed\n"; }
	return failures == 0 ? 0 : 1;
}
