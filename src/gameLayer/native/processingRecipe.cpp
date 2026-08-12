#include <native/processingRecipe.h>

#include <array>
#include <unordered_set>
#include <utility>

namespace mie::native
{
	namespace
	{
		bool validateStacks(const std::vector<ProcessingStack> &stacks,
			const ContentRegistry &content, std::string *error)
		{
			auto fail = [&](const char *message)
			{
				if (error) { *error = message; }
				return false;
			};

			// The v0.7 binary deliberately keeps processing recipes tiny so the
			// server can validate them cheaply and predictably.
			if (stacks.empty() || stacks.size() > 4)
			{
				return fail("processing recipe requires between one and four stacks");
			}

			std::unordered_set<std::string> uniqueStacks;
			for (const ProcessingStack &stack : stacks)
			{
				if (stack.kind != ContentKind::Block && stack.kind != ContentKind::Item)
				{
					return fail("processing stack must reference a block or item");
				}
				if (stack.count < 1 || stack.count > 999)
				{
					return fail("processing stack count is outside the supported range");
				}
				if (!content.resolve(stack.kind, stack.stableKey))
				{
					return fail("processing stack references unknown content");
				}

				const std::string key = std::to_string(static_cast<unsigned int>(stack.kind)) + ':' +
					stack.stableKey;
				if (!uniqueStacks.insert(key).second)
				{
					return fail("processing recipe contains a duplicate stack");
				}
			}
			return true;
		}

		ProcessingRecipe oreRecipe(const char *recipeKey, const char *blockKey,
			const char *itemKey)
		{
			ProcessingRecipe recipe;
			recipe.stableKey = recipeKey;
			recipe.machineKey = "mie:machine/prototype_processor";
			recipe.inputs.push_back({ContentKind::Block, blockKey, 2});
			recipe.outputs.push_back({ContentKind::Item, itemKey, 1});
			recipe.durationTicks = 160;
			return recipe;
		}
	}

	bool ProcessingRecipeRegistry::registerRecipe(ProcessingRecipe recipe,
		const ContentRegistry &content, std::string *error)
	{
		auto fail = [&](const char *message)
		{
			if (error) { *error = message; }
			return false;
		};

		if (keyToIndex.find(recipe.stableKey) != keyToIndex.end())
		{
			return fail("duplicate processing recipe key");
		}
		if (!content.resolve(ContentKind::Recipe, recipe.stableKey))
		{
			return fail("processing recipe key is not registered content");
		}
		if (!content.resolve(ContentKind::Machine, recipe.machineKey))
		{
			return fail("processing recipe references an unknown machine");
		}
		if (recipe.durationTicks < 1 || recipe.durationTicks > 72000)
		{
			return fail("processing duration is outside the supported range");
		}
		if (!validateStacks(recipe.inputs, content, error) ||
			!validateStacks(recipe.outputs, content, error))
		{
			return false;
		}

		const std::size_t index = recipes.size();
		keyToIndex.emplace(recipe.stableKey, index);
		recipes.push_back(std::move(recipe));
		return true;
	}

	const ProcessingRecipe *ProcessingRecipeRegistry::find(const std::string &stableKey) const
	{
		const auto found = keyToIndex.find(stableKey);
		if (found == keyToIndex.end()) { return nullptr; }
		return &recipes[found->second];
	}

	ProcessingRecipeRegistry createV07ProcessingRecipeRegistry(const ContentRegistry &content)
	{
		ProcessingRecipeRegistry registry;
		std::string error;

		const std::array<ProcessingRecipe, 5> oreRecipes = {
			oreRecipe("mie:recipe/process_copper_ore", "mie:block/v0.5/15", "mie:item/v0.5/2052"),
			oreRecipe("mie:recipe/process_lead_ore", "mie:block/v0.5/50", "mie:item/v0.5/2053"),
			oreRecipe("mie:recipe/process_iron_ore", "mie:block/v0.5/17", "mie:item/v0.5/2054"),
			oreRecipe("mie:recipe/process_silver_ore", "mie:block/v0.5/18", "mie:item/v0.5/2055"),
			oreRecipe("mie:recipe/process_gold_ore", "mie:block/v0.5/14", "mie:item/v0.5/2056"),
		};
		for (ProcessingRecipe recipe : oreRecipes)
		{
			registry.registerRecipe(std::move(recipe), content, &error);
		}

		ProcessingRecipe tin;
		tin.stableKey = "mie:recipe/refine_tin";
		tin.machineKey = "mie:machine/prototype_processor";
		tin.inputs = {
			{ContentKind::Item, "mie:item/tin_concentrate", 2},
			{ContentKind::Item, "mie:item/charcoal", 1},
		};
		tin.outputs = {{ContentKind::Item, "mie:item/tin_ingot", 1}};
		tin.durationTicks = 120;
		registry.registerRecipe(std::move(tin), content, &error);

		ProcessingRecipe bronze;
		bronze.stableKey = "mie:recipe/alloy_bronze";
		bronze.machineKey = "mie:machine/prototype_processor";
		bronze.inputs = {
			{ContentKind::Item, "mie:item/v0.5/2052", 3},
			{ContentKind::Item, "mie:item/tin_ingot", 1},
			{ContentKind::Item, "mie:item/charcoal", 1},
		};
		bronze.outputs = {{ContentKind::Item, "mie:item/bronze_ingot", 4}};
		bronze.durationTicks = 200;
		registry.registerRecipe(std::move(bronze), content, &error);

		return registry;
	}
}
