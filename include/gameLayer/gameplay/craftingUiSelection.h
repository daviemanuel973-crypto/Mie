#pragma once

namespace CraftingUiSelection
{
	inline int clampSlider(int slider, int recipeCount)
	{
		if (recipeCount <= 0) { return -1; }
		const int maxSlider = recipeCount > 1 ? recipeCount - 2 : -1;
		if (slider < -1) { return -1; }
		if (slider > maxSlider) { return maxSlider; }
		return slider;
	}

	inline bool isValidRecipeIndex(int index, int recipeCount)
	{
		return index >= 0 && index < recipeCount;
	}

	inline int selectedRecipeIndex(int slider, int recipeCount)
	{
		if (recipeCount <= 0) { return -1; }
		const int index = clampSlider(slider, recipeCount) + 1;
		return isValidRecipeIndex(index, recipeCount) ? index : -1;
	}
}
