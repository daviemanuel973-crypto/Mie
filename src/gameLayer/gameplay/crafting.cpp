#include <gameplay/crafting.h>
#include <gameplay/fieldGuide.h>
#include <gameplay/itemDurability.h>


template<long long I>
CraftingRecepie recepie(Item result, std::array<Item, I> items, bool applyItemCreator = 0)
{
	CraftingRecepie ret;
	if (applyItemCreator)
	{
		ret.result = itemCreator(result.type);
		ret.result.counter = result.counter;
	}
	else
	{
		ret.result = result;
	}

	for (std::size_t i = 0; i < sizeof(items) / sizeof(items[0]); i++)
	{
		ret.items[i] = items[i];
	}
	return ret;
}


static CraftingRecepie recepies[] =
{
	//basic
	recepie<1>(Item(BlockTypes::wooden_plank, 4), {Item(BlockTypes::woodLog)}),
	recepie<1>(Item(BlockTypes::wooden_plank, 4), {Item(BlockTypes::spruce_log)}),
	recepie<1>(Item(BlockTypes::birchPlanks, 4), {Item(BlockTypes::birch_log)}),
	recepie<1>(Item(BlockTypes::jungle_planks, 4),{Item(BlockTypes::jungle_log)}),
	recepie<1>(Item(BlockTypes::sand_stone, 1),{Item(BlockTypes::sand, 2)}),
	recepie<1>(Item(BlockTypes::hardSandStone, 1),{Item(BlockTypes::sand, 2)}),
	recepie<1>(Item(BlockTypes::stoneBrick, 1),{Item(BlockTypes::stone, 2)}),

	// survival progression
	recepie<1>(Item(ItemTypes::stick, 4), {Item(BlockTypes::wooden_plank, 2)}).setAnyWood(),
	recepie<1>(Item(BlockTypes::workBench, 1),{Item(BlockTypes::wooden_plank, 4)}).setAnyWood(),
	recepie<1>(Item(BlockTypes::furnace, 1),{Item(BlockTypes::cobblestone, 8)}).setRequiresWorkBench(),
	recepie<2>(Item(BlockTypes::cookingPot, 1),{Item(ItemTypes::ironIngot, 5), Item(BlockTypes::wooden_plank, 2)}).setAnyWood().setRequiresWorkBench(),
	recepie<1>(Item(BlockTypes::woddenChest, 1), {Item(BlockTypes::wooden_plank, 8)}).setAnyWood().setRequiresWorkBench(),
	recepie<1>(Item(ItemTypes::bandage, 1), {Item(ItemTypes::cloth, 3)}),
	// Shipped v0.7: the guide is also craftable, even though new players receive one automatically.
	recepie<2>(Item(ItemTypes::fieldGuide, 1), {Item(ItemTypes::cloth, 2), Item(ItemTypes::wheat, 1)}),
	recepie<2>(Item(BlockTypes::reinforcedBarricade, 2), {Item(BlockTypes::wooden_plank, 6), Item(ItemTypes::stick, 2)}).setAnyWood().setRequiresWorkBench(),
	recepie<2>(Item(BlockTypes::woodenSpikeTrap, 2), {Item(BlockTypes::wooden_plank, 4), Item(ItemTypes::stick, 4)}).setAnyWood().setRequiresWorkBench(),

	// legacy alternatives stay available
	recepie<1>(Item(BlockTypes::workBench, 1),{Item(BlockTypes::wooden_plank, 10)}).setAnyWood(),
	recepie<3>(Item(BlockTypes::furnace, 1),{Item(BlockTypes::cobblestone, 20), Item(BlockTypes::torch, 3), Item(BlockTypes::wooden_plank, 4)}).setAnyWood().setRequiresWorkBench(),

	//furniture
	recepie<1>(Item(BlockTypes::oakChair, 1),{Item(BlockTypes::wooden_plank, 4)}).setRequiresWorkBench(),
	recepie<1>(Item(BlockTypes::oakBigChair, 1),{Item(BlockTypes::wooden_plank, 6)}).setRequiresWorkBench(),
	recepie<1>(Item(BlockTypes::oakLogChair, 1),{Item(BlockTypes::woodLog, 4)}).setRequiresWorkBench(),
	recepie<1>(Item(BlockTypes::oakLogBigChair, 1),{Item(BlockTypes::woodLog, 6)}).setRequiresWorkBench(),
	recepie<1>(Item(BlockTypes::oakTable, 1),{Item(BlockTypes::wooden_plank, 8)}).setRequiresWorkBench(),
	recepie<1>(Item(BlockTypes::oakLogTable, 1), {Item(BlockTypes::woodLog, 8)}).setRequiresWorkBench(),
	recepie<2>(Item(BlockTypes::woddenChest, 1), {Item(BlockTypes::wooden_plank, 8), Item(ItemTypes::leadIngot, 2)}).setRequiresWorkBench(),
	recepie<2>(Item(BlockTypes::woddenChest, 1), {Item(BlockTypes::wooden_plank, 8), Item(ItemTypes::ironIngot, 1)}).setRequiresWorkBench(),

	//todo require goblin loom
	recepie<3>(Item(BlockTypes::goblinWorkBench, 1),{Item(BlockTypes::wooden_plank, 10), Item(ItemTypes::cloth, 5), Item(ItemTypes::fang, 2)}).setRequiresGoblin(),
	recepie<3>(Item(BlockTypes::goblinChair, 1),{Item(BlockTypes::wooden_plank, 4), Item(ItemTypes::cloth, 2), Item(ItemTypes::fang, 1)}).setRequiresGoblin(),
	recepie<3>(Item(BlockTypes::goblinTable, 1),{Item(BlockTypes::wooden_plank, 8), Item(ItemTypes::cloth, 4), Item(ItemTypes::fang, 2)}).setRequiresGoblin(),
	recepie<3>(Item(BlockTypes::goblinTorch, 4),{Item(BlockTypes::wooden_plank, 1), Item(ItemTypes::cloth, 1), Item(ItemTypes::fang, 1)}).setAnyWood().setRequiresGoblin(),
	recepie<4>(Item(BlockTypes::goblinChest, 1),{Item(BlockTypes::wooden_plank, 8), Item(ItemTypes::cloth, 5), Item(ItemTypes::fang, 2), Item(ItemTypes::leadIngot, 2)}).setRequiresGoblin(),
	recepie<4>(Item(BlockTypes::goblinChest, 1),{Item(BlockTypes::wooden_plank, 8), Item(ItemTypes::cloth, 5), Item(ItemTypes::fang, 2), Item(ItemTypes::ironIngot, 1)}).setRequiresGoblin(),

	//food / exploration provisions
	recepie<2>(Item(ItemTypes::applePie, 1), {Item(ItemTypes::apple, 2), Item(ItemTypes::wheat, 3)}).setRequiresCookingPot(),
	recepie<1>(Item(BlockTypes::hayBalde, 1), {Item(ItemTypes::wheat, 9)}).setRequiresWorkBench(),
	recepie<1>(Item(ItemTypes::wheat, 9), {Item(BlockTypes::hayBalde, 1)}),

	//coins
	recepie<1>(Item(ItemTypes::silverCoin, 1), {Item(ItemTypes::copperCoin, 100)}),
	recepie<1>(Item(ItemTypes::goldCoin, 1), {Item(ItemTypes::silverCoin, 100)}),
	recepie<1>(Item(ItemTypes::diamondCoin, 1), {Item(ItemTypes::goldCoin, 100)}),
	recepie<1>(Item(ItemTypes::goldCoin, 100), {Item(ItemTypes::diamondCoin, 1)}),
	recepie<1>(Item(ItemTypes::silverCoin, 100), {Item(ItemTypes::goldCoin, 1)}),
	recepie<1>(Item(ItemTypes::copperCoin, 100), {Item(ItemTypes::silverCoin, 1)}),

	recepie<2>(Item(BlockTypes::torchWood, 4), {Item(BlockTypes::wooden_plank, 1), Item(ItemTypes::cloth, 1)}).setAnyWood(),
	// Shipped v0.7 guide recipe: 1 charcoal + 1 stick -> 4 torches.
	recepie<2>(Item(BlockTypes::torchWood, 4), {Item(ItemTypes::charcoal, 1), Item(ItemTypes::stick, 1)}),

	//arrows
	recepie<2>(Item(ItemTypes::arrow, 10), {Item(BlockTypes::wooden_plank, 1), Item(BlockTypes::cobblestone, 1)}).setAnyWood(),
	recepie<2>(Item(ItemTypes::flamingArrow, 5), {Item(ItemTypes::arrow, 5), Item(BlockTypes::torchWood, 1)}),
	recepie<2>(Item(ItemTypes::goblinArrow, 5), {Item(ItemTypes::arrow, 5), Item(ItemTypes::fang, 1)}),
	recepie<2>(Item(ItemTypes::boneArrow, 5), {Item(ItemTypes::arrow, 5), Item(ItemTypes::bone, 1)}),

	//bars and v0.7 bronze-age progression
	recepie<1>(Item(ItemTypes::copperIngot, 1), {Item(BlockTypes::copperOre, 2)}).setRequiresFurnace(),
	recepie<1>(Item(ItemTypes::leadIngot, 1), {Item(BlockTypes::leadOre, 2)}).setRequiresFurnace(),
	recepie<1>(Item(ItemTypes::ironIngot, 1), {Item(BlockTypes::ironOre, 3)}).setRequiresFurnace(),
	recepie<1>(Item(ItemTypes::silverIngot, 1), {Item(BlockTypes::silverOre, 3)}).setRequiresFurnace(),
	recepie<1>(Item(ItemTypes::goldIngot, 1), {Item(BlockTypes::goldOre, 4)}).setRequiresFurnace(),
	// Exact contracts recovered from the shipped v0.7 executable.
	recepie<1>(Item(ItemTypes::charcoal, 2), {Item(BlockTypes::woodLog, 1)}).setAnyWood().setRequiresFurnace(),
	recepie<1>(Item(ItemTypes::tinConcentrate, 1), {Item(BlockTypes::gravel, 8)}).setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::tinIngot, 1), {Item(ItemTypes::tinConcentrate, 2), Item(ItemTypes::charcoal, 1)}).setRequiresFurnace(),
	recepie<3>(Item(ItemTypes::bronzeIngot, 4), {Item(ItemTypes::copperIngot, 3), Item(ItemTypes::tinIngot, 1), Item(ItemTypes::charcoal, 1)}).setRequiresFurnace(),

	//tools
	recepie<2>(Item(ItemTypes::copperPickaxe, 1), {Item(ItemTypes::copperIngot, 4), Item(BlockTypes::wooden_plank, 3)}).setAnyWood().setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::copperAxe, 1), {Item(ItemTypes::copperIngot, 3), Item(BlockTypes::wooden_plank, 3)}).setAnyWood().setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::copperShovel, 1), {Item(ItemTypes::copperIngot, 3), Item(BlockTypes::wooden_plank, 3)}).setAnyWood().setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::leadPickaxe, 1), {Item(ItemTypes::leadIngot, 4), Item(BlockTypes::wooden_plank, 3)}).setAnyWood().setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::leadAxe, 1), {Item(ItemTypes::leadIngot, 3), Item(BlockTypes::wooden_plank, 3)}).setAnyWood().setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::leadShovel, 1), {Item(ItemTypes::leadIngot, 3), Item(BlockTypes::wooden_plank, 3)}).setAnyWood().setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::ironPickaxe, 1), {Item(ItemTypes::ironIngot, 4), Item(BlockTypes::wooden_plank, 3)}).setAnyWood().setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::ironAxe, 1), {Item(ItemTypes::ironIngot, 3), Item(BlockTypes::wooden_plank, 3)}).setAnyWood().setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::ironShovel, 1), {Item(ItemTypes::ironIngot, 3), Item(BlockTypes::wooden_plank, 3)}).setAnyWood().setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::silverPickaxe, 1), {Item(ItemTypes::silverIngot, 4), Item(BlockTypes::wooden_plank, 3)}).setAnyWood().setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::silverAxe, 1), {Item(ItemTypes::silverIngot, 3), Item(BlockTypes::wooden_plank, 3)}).setAnyWood().setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::silverShovel, 1), {Item(ItemTypes::silverIngot, 3), Item(BlockTypes::wooden_plank, 3)}).setAnyWood().setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::goldPickaxe, 1), {Item(ItemTypes::goldIngot, 4), Item(BlockTypes::wooden_plank, 3)}).setAnyWood().setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::goldAxe, 1), {Item(ItemTypes::goldIngot, 3), Item(BlockTypes::wooden_plank, 3)}).setAnyWood().setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::goldShovel, 1), {Item(ItemTypes::goldIngot, 3), Item(BlockTypes::wooden_plank, 3)}).setAnyWood().setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::bronzePickaxe, 1), {Item(ItemTypes::bronzeIngot, 4), Item(ItemTypes::stick, 2)}).setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::bronzeAxe, 1), {Item(ItemTypes::bronzeIngot, 3), Item(ItemTypes::stick, 2)}).setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::bronzeShovel, 1), {Item(ItemTypes::bronzeIngot, 2), Item(ItemTypes::stick, 2)}).setRequiresWorkBench(),

	//weapons
	recepie<2>(Item(ItemTypes::copperSword, 1), {Item(ItemTypes::copperIngot, 4), Item(ItemTypes::stick, 1)}).setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::leadSword, 1), {Item(ItemTypes::leadIngot, 4), Item(ItemTypes::stick, 1)}).setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::ironSword, 1), {Item(ItemTypes::ironIngot, 4), Item(ItemTypes::stick, 1)}).setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::silverSword, 1), {Item(ItemTypes::silverIngot, 4), Item(ItemTypes::stick, 1)}).setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::goldSword, 1), {Item(ItemTypes::goldIngot, 4), Item(ItemTypes::stick, 1)}).setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::bronzeSword, 1), {Item(ItemTypes::bronzeIngot, 3), Item(ItemTypes::stick, 1)}).setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::copperWarHammer, 1), {Item(ItemTypes::copperIngot, 5), Item(ItemTypes::stick, 2)}).setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::copperSpear, 1), {Item(ItemTypes::copperIngot, 3), Item(ItemTypes::stick, 2)}).setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::copperKnife, 1), {Item(ItemTypes::copperIngot, 2), Item(ItemTypes::stick, 1)}).setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::copperBattleAxe, 1), {Item(ItemTypes::copperIngot, 5), Item(ItemTypes::stick, 2)}).setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::ironWarHammer, 1), {Item(ItemTypes::ironIngot, 5), Item(ItemTypes::stick, 2)}).setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::ironSpear, 1), {Item(ItemTypes::ironIngot, 3), Item(ItemTypes::stick, 2)}).setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::ironKnife, 1), {Item(ItemTypes::ironIngot, 2), Item(ItemTypes::stick, 1)}).setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::ironBattleAxe, 1), {Item(ItemTypes::ironIngot, 5), Item(ItemTypes::stick, 2)}).setRequiresWorkBench(),

	//armour
	recepie<1>(Item(ItemTypes::copperHelmet, 1), {Item(ItemTypes::copperIngot, 6)}).setRequiresWorkBench(),
	recepie<1>(Item(ItemTypes::copperChestPlate, 1), {Item(ItemTypes::copperIngot, 8)}).setRequiresWorkBench(),
	recepie<1>(Item(ItemTypes::copperBoots, 1), {Item(ItemTypes::copperIngot, 5)}).setRequiresWorkBench(),
	recepie<1>(Item(ItemTypes::leadHelmet, 1), {Item(ItemTypes::leadIngot, 6)}).setRequiresWorkBench(),
	recepie<1>(Item(ItemTypes::leadChestPlate, 1), {Item(ItemTypes::leadIngot, 8)}).setRequiresWorkBench(),
	recepie<1>(Item(ItemTypes::leadBoots, 1), {Item(ItemTypes::leadIngot, 5)}).setRequiresWorkBench(),
	recepie<1>(Item(ItemTypes::ironHelmet, 1), {Item(ItemTypes::ironIngot, 6)}).setRequiresWorkBench(),
	recepie<1>(Item(ItemTypes::ironChestPlate, 1), {Item(ItemTypes::ironIngot, 8)}).setRequiresWorkBench(),
	recepie<1>(Item(ItemTypes::ironBoots, 1), {Item(ItemTypes::ironIngot, 5)}).setRequiresWorkBench(),
	recepie<1>(Item(ItemTypes::silverHelmet, 1), {Item(ItemTypes::silverIngot, 6)}).setRequiresWorkBench(),
	recepie<1>(Item(ItemTypes::silverChestPlate, 1), {Item(ItemTypes::silverIngot, 8)}).setRequiresWorkBench(),
	recepie<1>(Item(ItemTypes::silverBoots, 1), {Item(ItemTypes::silverIngot, 5)}).setRequiresWorkBench(),
	recepie<1>(Item(ItemTypes::goldHelmet, 1), {Item(ItemTypes::goldIngot, 6)}).setRequiresWorkBench(),
	recepie<1>(Item(ItemTypes::goldChestPlate, 1), {Item(ItemTypes::goldIngot, 8)}).setRequiresWorkBench(),
	recepie<1>(Item(ItemTypes::goldBoots, 1), {Item(ItemTypes::goldIngot, 5)}).setRequiresWorkBench(),

	// v0.9 append-only recipes. Keeping these after the 103 shipped recipes
	// preserves every legacy recipe index used by network crafting packets.
	recepie<2>(Item(ItemTypes::bedroll, 1), {Item(ItemTypes::cloth, 6), Item(BlockTypes::hayBalde, 1)}).setRequiresWorkBench(),
	recepie<2>(Item(ItemTypes::bronzePickaxe, 1), {Item(ItemTypes::bronzePickaxe, 1), Item(ItemTypes::bronzeIngot, 1)}).setRequiresWorkBench().setRepairsDurableItem(),
	recepie<2>(Item(ItemTypes::bronzeAxe, 1), {Item(ItemTypes::bronzeAxe, 1), Item(ItemTypes::bronzeIngot, 1)}).setRequiresWorkBench().setRepairsDurableItem(),
	recepie<2>(Item(ItemTypes::bronzeShovel, 1), {Item(ItemTypes::bronzeShovel, 1), Item(ItemTypes::bronzeIngot, 1)}).setRequiresWorkBench().setRepairsDurableItem(),
	recepie<2>(Item(ItemTypes::bronzeSword, 1), {Item(ItemTypes::bronzeSword, 1), Item(ItemTypes::bronzeIngot, 1)}).setRequiresWorkBench().setRepairsDurableItem(),
};

constexpr int LegacyCraftingRecipeCount = 103;
static_assert(sizeof(recepies) / sizeof(recepies[0]) == 108,
	"v0.9 crafting recipes changed unexpectedly");

int getCraftingRecipeCount()
{
	return static_cast<int>(sizeof(recepies) / sizeof(recepies[0]));
}


std::vector<CraftingRecepieIndex> getAllPossibleRecepies(PlayerInventory &playerInventory, int craftingStation)
{
	std::vector<CraftingRecepieIndex> rez;
	rez.reserve(sizeof(recepies) / sizeof(recepies[0]));
	for (int i = 0; i < getCraftingRecipeCount(); i++)
	{
		if (canItemBeCrafted(recepies[i], playerInventory))
		{
			bool good = true;
			if (recepies[i].requiresWorkBench && craftingStation != WorkStationType::WorkStationType_WorkBench) { good = false; }
			if (recepies[i].requiresFurnace && craftingStation != WorkStationType::WorkStationType_Furnace) { good = false; }
			if (recepies[i].requiresGoblin && craftingStation != WorkStationType::WorkStationType_GoblinStitchingPost) { good = false; }
			if (recepies[i].requiresCookingPot && craftingStation != WorkStationType::WorkStationType_CookingPot) { good = false; }
			int benchesRequired = 0;
			benchesRequired += recepies[i].requiresWorkBench + recepies[i].requiresFurnace + recepies[i].requiresGoblin;
			assert(benchesRequired <= 1);
			if (good) { rez.push_back({recepies[i], i}); }
		}
	}
	return rez;
}


bool recepieExists(int recepieIndex)
{
	if (recepieIndex < 0) { return 0; }
	if (recepieIndex >= getCraftingRecipeCount()) { return 0; }
	return 1;
}

namespace
{
	bool knowsAnyMatchingWood(const RecipeDiscovery &discovery, bool logs)
	{
		for (std::uint16_t type = 1; type < BlockTypes::BlocksCount; ++type)
		{
			if (!discovery.knowsType(type)) { continue; }
			if (logs ? isGuideWoodLogType(type) : isWoodPlank(type)) { return true; }
		}
		return false;
	}

	bool ingredientIsDiscovered(const CraftingRecepie &recepie, const Item &ingredient,
		const RecipeDiscovery &discovery)
	{
		if (discovery.knowsType(ingredient.type)) { return true; }
		if (!recepie.anyWood) { return false; }
		if (isWoodPlank(ingredient.type)) { return knowsAnyMatchingWood(discovery, false); }
		if (isGuideWoodLogType(ingredient.type)) { return knowsAnyMatchingWood(discovery, true); }
		return false;
	}
}

bool isCraftingRecipeDiscovered(int recepieIndex, const RecipeDiscovery &discovery)
{
	if (!recepieExists(recepieIndex)) { return false; }
	const CraftingRecepie &recepie = recepies[recepieIndex];
	for (const Item &ingredient : recepie.items)
	{
		if (ingredient.type == 0) { break; }
		if (!ingredientIsDiscovered(recepie, ingredient, discovery)) { return false; }
	}
	return true;
}

std::vector<CraftingRecepieIndex> getDiscoveredCraftingRecipes(const RecipeDiscovery &discovery)
{
	std::vector<CraftingRecepieIndex> result;
	result.reserve(getCraftingRecipeCount());
	for (int index = 0; index < getCraftingRecipeCount(); ++index)
	{
		if (isCraftingRecipeDiscovered(index, discovery))
		{
			result.push_back({recepies[index], index});
		}
	}
	return result;
}

const char *getCraftingRecipeStationName(const CraftingRecepie &recepie)
{
	if (recepie.requiresWorkBench) { return "WORKBENCH"; }
	if (recepie.requiresFurnace) { return "FURNACE"; }
	if (recepie.requiresGoblin) { return "GOBLIN STATION"; }
	if (recepie.requiresCookingPot) { return "COOKING POT"; }
	return "HAND CRAFTING";
}


CraftingRecepie getRecepieFromIndexUnsafe(int recepieIndex)
{
	return recepies[recepieIndex];
}


namespace
{
	bool matchesAnyWoodRule(const CraftingRecepie &recepie, const Item &needed, const Item &available)
	{
		if (!recepie.anyWood) { return false; }
		if (isWoodPlank(needed.type) && isWoodPlank(available.type)) { return true; }
		if (isGuideWoodLogType(needed.type) && isGuideWoodLogType(available.type)) { return true; }
		return false;
	}

	bool matchesRepairRule(const CraftingRecepie &recepie, const Item &needed, const Item &available)
	{
		if (!recepie.repairsDurableItem || needed.type != available.type ||
			!itemUsesDurability(needed.type))
		{
			return false;
		}
		return getRemainingItemDurability(available.type, available.metaData) <
			getMaximumItemDurability(available.type);
	}

	bool ingredientMatches(const CraftingRecepie &recepie, Item &needed, Item &available)
	{
		// Repair recipes must only accept a damaged durable item. An undamaged
		// item may be metadata-identical to the recipe prototype, so checking
		// the generic equality rule first would bypass the durability guard.
		if (recepie.repairsDurableItem && needed.type == available.type &&
			itemUsesDurability(needed.type))
		{
			return matchesRepairRule(recepie, needed, available);
		}

		return areItemsTheSame(available, needed) ||
			matchesAnyWoodRule(recepie, needed, available);
	}
}


bool canItemBeCrafted(CraftingRecepie &recepie, PlayerInventory &inventory)
{
	Item neededItems[sizeof(recepie.items) / sizeof(recepie.items[0])];
	for (std::size_t i = 0; i < sizeof(recepie.items) / sizeof(recepie.items[0]); i++) { neededItems[i] = recepie.items[i]; }

	for (std::size_t i = 0; i < sizeof(recepie.items) / sizeof(recepie.items[0]); i++)
	{
		if (neededItems[i].type == 0) { break; }
		for (int j = 0; j < PlayerInventory::INVENTORY_CAPACITY; j++)
		{
			if (ingredientMatches(recepie, neededItems[i], inventory.items[j]))
			{
				if (neededItems[i].counter <= inventory.items[j].counter)
				{
					neededItems[i] = Item();
					break;
				}
				neededItems[i].counter -= inventory.items[j].counter;
			}
		}
	}

	for (std::size_t i = 0; i < sizeof(recepie.items) / sizeof(recepie.items[0]); i++)
	{
		if (neededItems[i].type != 0) { return false; }
	}
	return true;
}


void craftItemUnsafe(CraftingRecepie &recepie, PlayerInventory &inventory)
{
	Item neededItems[sizeof(recepie.items) / sizeof(recepie.items[0])];
	for (std::size_t i = 0; i < sizeof(recepie.items) / sizeof(recepie.items[0]); i++) { neededItems[i] = recepie.items[i]; }

	for (std::size_t i = 0; i < sizeof(recepie.items) / sizeof(recepie.items[0]); i++)
	{
		if (neededItems[i].type == 0) { break; }
		for (int j = 0; j < PlayerInventory::INVENTORY_CAPACITY; j++)
		{
			if (ingredientMatches(recepie, neededItems[i], inventory.items[j]))
			{
				if (neededItems[i].counter == inventory.items[j].counter)
				{
					neededItems[i] = Item();
					inventory.items[j] = Item();
					break;
				}
				if (neededItems[i].counter < inventory.items[j].counter)
				{
					inventory.items[j].counter -= neededItems[i].counter;
					neededItems[i] = Item();
					break;
				}
				neededItems[i].counter -= inventory.items[j].counter;
				inventory.items[j] = Item();
			}
		}
	}
}