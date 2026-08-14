#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

struct FurnaceStack
{
	std::uint16_t type = 0;
	std::uint16_t count = 0;
};

struct FurnaceIngredient
{
	std::uint16_t type = 0;
	std::uint16_t count = 0;
	bool anyWoodLog = false;
};

struct FurnaceRecipeDefinition
{
	std::array<FurnaceIngredient, 3> inputs = {};
	FurnaceStack output = {};
	float durationSeconds = 0.f;
};

constexpr std::size_t FURNACE_RECIPE_COUNT = 8;

const std::array<FurnaceRecipeDefinition, FURNACE_RECIPE_COUNT> &getFurnaceRecipes();

bool isFurnaceWoodLog(std::uint16_t type);
bool isFurnaceFuel(std::uint16_t type);
float getFurnaceFuelSeconds(std::uint16_t type);
bool isFurnaceInput(std::uint16_t type);

// Returns -1 when the three input cells cannot currently satisfy a recipe.
int findFurnaceRecipe(const std::array<FurnaceStack, 3> &inputs);
