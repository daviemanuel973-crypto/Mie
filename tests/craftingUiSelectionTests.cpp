#include <gameplay/craftingUiSelection.h>
#include <cassert>
#include <iostream>

int main()
{
	using namespace CraftingUiSelection;
	assert(clampSlider(0, 0) == -1);
	assert(selectedRecipeIndex(0, 0) == -1);
	assert(clampSlider(0, 1) == -1);
	assert(clampSlider(100, 1) == -1);
	assert(selectedRecipeIndex(0, 1) == 0);
	assert(selectedRecipeIndex(100, 1) == 0);
	assert(!isValidRecipeIndex(-1, 1));
	assert(isValidRecipeIndex(0, 1));
	assert(!isValidRecipeIndex(1, 1));
	assert(clampSlider(-50, 2) == -1);
	assert(clampSlider(0, 2) == 0);
	assert(clampSlider(50, 2) == 0);
	assert(selectedRecipeIndex(-1, 2) == 0);
	assert(selectedRecipeIndex(0, 2) == 1);
	assert(clampSlider(100, 8) == 6);
	assert(selectedRecipeIndex(100, 8) == 7);
	std::cout << "crafting UI selection bounds: ok\n";
	return 0;
}
