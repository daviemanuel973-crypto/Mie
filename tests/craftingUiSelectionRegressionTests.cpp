#include <cassert>
#include <iostream>

namespace
{
	int clampSlider(int slider, int recipeCount)
	{
		if (recipeCount <= 0)
		{
			return -1;
		}

		const int maxSlider = recipeCount - 2;
		if (slider < -1)
		{
			return -1;
		}
		if (slider > maxSlider)
		{
			return maxSlider;
		}
		return slider;
	}

	int selectedRecipeIndex(int slider, int recipeCount)
	{
		if (recipeCount <= 0)
		{
			return -1;
		}
		return clampSlider(slider, recipeCount) + 1;
	}
}

int main()
{
	// Empty crafting lists must never produce a valid selection.
	assert(clampSlider(0, 0) == -1);
	assert(selectedRecipeIndex(0, 0) == -1);

	// Regression: one recipe must clamp the slider to -1 so index 0 is selected.
	assert(clampSlider(0, 1) == -1);
	assert(clampSlider(100, 1) == -1);
	assert(clampSlider(-100, 1) == -1);
	assert(selectedRecipeIndex(0, 1) == 0);
	assert(selectedRecipeIndex(100, 1) == 0);

	// Two recipes retain both valid selections without escaping the vector.
	assert(clampSlider(-50, 2) == -1);
	assert(clampSlider(0, 2) == 0);
	assert(clampSlider(50, 2) == 0);
	assert(selectedRecipeIndex(-1, 2) == 0);
	assert(selectedRecipeIndex(0, 2) == 1);

	// Larger lists remain bounded to [0, recipeCount - 1].
	for (int recipeCount = 1; recipeCount <= 128; ++recipeCount)
	{
		for (int slider = -256; slider <= 256; ++slider)
		{
			const int index = selectedRecipeIndex(slider, recipeCount);
			assert(index >= 0);
			assert(index < recipeCount);
		}
	}

	std::cout << "crafting UI selection bounds: ok\n";
	return 0;
}
