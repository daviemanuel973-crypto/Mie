#include <gameplay/furnaceRecipes.h>

#include <blocks.h>
#include <gameplay/items.h>

namespace
{
	constexpr FurnaceIngredient ingredient(std::uint16_t type, std::uint16_t count,
		bool anyWoodLog = false)
	{
		return {type, count, anyWoodLog};
	}

	const std::array<FurnaceRecipeDefinition, FURNACE_RECIPE_COUNT> furnaceRecipes = {{
		{{ingredient(BlockTypes::copperOre, 2)}, {ItemTypes::copperIngot, 1}, 8.f},
		{{ingredient(BlockTypes::leadOre, 2)}, {ItemTypes::leadIngot, 1}, 8.f},
		{{ingredient(BlockTypes::ironOre, 3)}, {ItemTypes::ironIngot, 1}, 8.f},
		{{ingredient(BlockTypes::silverOre, 3)}, {ItemTypes::silverIngot, 1}, 8.f},
		{{ingredient(BlockTypes::goldOre, 4)}, {ItemTypes::goldIngot, 1}, 8.f},
		{{ingredient(BlockTypes::woodLog, 1, true)}, {ItemTypes::charcoal, 2}, 6.f},
		{{ingredient(ItemTypes::tinConcentrate, 2), ingredient(ItemTypes::charcoal, 1)},
			{ItemTypes::tinIngot, 1}, 6.f},
		{{ingredient(ItemTypes::copperIngot, 3), ingredient(ItemTypes::tinIngot, 1),
			ingredient(ItemTypes::charcoal, 1)}, {ItemTypes::bronzeIngot, 4}, 10.f},
	}};

	bool matches(const FurnaceIngredient &needed, const FurnaceStack &available)
	{
		if (needed.type == 0 || available.type == 0) { return false; }
		if (needed.anyWoodLog) { return isFurnaceWoodLog(available.type); }
		return needed.type == available.type;
	}
}

const std::array<FurnaceRecipeDefinition, FURNACE_RECIPE_COUNT> &getFurnaceRecipes()
{
	return furnaceRecipes;
}

bool isFurnaceWoodLog(std::uint16_t type)
{
	return type == BlockTypes::woodLog || type == BlockTypes::birch_log ||
		type == BlockTypes::jungle_log || type == BlockTypes::palm_log ||
		type == BlockTypes::spruce_log || type == BlockTypes::strippedOakLog ||
		type == BlockTypes::strippedBirchLog || type == BlockTypes::strippedSpruceLog;
}

bool isFurnaceFuel(std::uint16_t type)
{
	return getFurnaceFuelSeconds(type) > 0.f;
}

float getFurnaceFuelSeconds(std::uint16_t type)
{
	if (type == ItemTypes::charcoal) { return 16.f; }
	if (isFurnaceWoodLog(type)) { return 8.f; }
	if (type == ItemTypes::stick) { return 1.f; }
	if (type == BlockTypes::wooden_plank || type == BlockTypes::jungle_planks ||
		type == BlockTypes::sprucePlank || type == BlockTypes::birchPlanks)
	{
		return 3.f;
	}
	return 0.f;
}

bool isFurnaceInput(std::uint16_t type)
{
	if (type == 0) { return true; }
	for (const auto &recipe : furnaceRecipes)
	{
		for (const auto &needed : recipe.inputs)
		{
			if (needed.type == 0) { break; }
			if ((needed.anyWoodLog && isFurnaceWoodLog(type)) || needed.type == type)
			{
				return true;
			}
		}
	}
	return false;
}

int findFurnaceRecipe(const std::array<FurnaceStack, 3> &inputs)
{
	for (std::size_t recipeIndex = 0; recipeIndex < furnaceRecipes.size(); ++recipeIndex)
	{
		const auto &recipe = furnaceRecipes[recipeIndex];
		bool used[3] = {};
		bool complete = true;

		for (const auto &needed : recipe.inputs)
		{
			if (needed.type == 0) { break; }
			bool found = false;
			for (std::size_t slot = 0; slot < inputs.size(); ++slot)
			{
				if (!used[slot] && matches(needed, inputs[slot]) && inputs[slot].count >= needed.count)
				{
					used[slot] = true;
					found = true;
					break;
				}
			}
			if (!found)
			{
				complete = false;
				break;
			}
		}

		if (complete) { return static_cast<int>(recipeIndex); }
	}
	return -1;
}
