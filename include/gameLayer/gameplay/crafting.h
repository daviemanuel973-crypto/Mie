#pragma once
#include <gameplay/items.h>
#include <array>



struct CraftingRecepie
{

	Item items[5] = {};
	Item result = {};

	bool anyWood = 0;
	bool requiresWorkBench = 0;
	bool requiresFurnace = 0;
	bool requiresGoblin = 0;
	bool requiresCookingPot = 0;
	// v0.9: repair recipes may consume a damaged durable item regardless of its
	// durability metadata. Normal recipes retain exact metadata matching.
	bool repairsDurableItem = 0;

	CraftingRecepie() {};

	CraftingRecepie &setAnyWood() { anyWood = true; return *this; }
	CraftingRecepie &setRequiresWorkBench() { requiresWorkBench = true; return *this; }
	CraftingRecepie &setRequiresGoblin() { requiresGoblin = true; return *this; }
	CraftingRecepie &setRequiresCookingPot() { requiresCookingPot = true; return *this; }
	CraftingRecepie &setRequiresFurnace() { requiresFurnace = true; return *this; }
	CraftingRecepie &setRepairsDurableItem()
	{
		repairsDurableItem = true;
		// Repair recipes place the durable tool in the first ingredient slot. Give
		// that recipe-side ingredient a non-empty marker so a pristine tool (whose
		// canonical full-durability representation has empty metadata) cannot pass
		// the normal exact-item comparison before the dedicated repair rule checks
		// that the supplied tool is actually damaged. The marker is never persisted
		// or emitted as the crafted result; it only disambiguates recipe matching.
		if (items[0].type != 0) { items[0].metaData = {0}; }
		return *this;
	}
};

struct CraftingRecepieIndex
{
	CraftingRecepie recepie;
	int index = 0;
};


std::vector<CraftingRecepieIndex> getAllPossibleRecepies(PlayerInventory &playerInventory, int craftingStation);

// Recipe-book queries never change the legacy recipe indexes used by crafting
// packets. Discovery is knowledge-only; the server still validates materials
// and the required station when a craft is requested.
int getCraftingRecipeCount();
bool isCraftingRecipeDiscovered(int recepieIndex, const RecipeDiscovery &discovery);
std::vector<CraftingRecepieIndex> getDiscoveredCraftingRecipes(const RecipeDiscovery &discovery);
const char *getCraftingRecipeStationName(const CraftingRecepie &recepie);


bool recepieExists(int recepieIndex);

CraftingRecepie getRecepieFromIndexUnsafe(int recepieIndex);

bool canItemBeCrafted(CraftingRecepie &recepie, PlayerInventory &inventory);


//removes items from the inventory in order to craft the recepie
void craftItemUnsafe(CraftingRecepie &recepie, PlayerInventory &inventory);
