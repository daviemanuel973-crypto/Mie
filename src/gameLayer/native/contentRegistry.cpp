#include <native/contentRegistry.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <limits>
#include <sstream>

namespace mie::native
{
	namespace
	{
		bool validStableKey(const std::string &key)
		{
			if (key.size() < 5 || key.size() > 96 || key.find(':') == std::string::npos)
			{
				return false;
			}
			return std::all_of(key.begin(), key.end(), [](unsigned char character)
			{
				return std::islower(character) || std::isdigit(character) ||
					character == ':' || character == '/' || character == '_' || character == '-' ||
					character == '.';
			});
		}

		std::string legacyKey(const char *kind, std::uint32_t id)
		{
			std::ostringstream stream;
			stream << "mie:" << kind << "/v0.5/" << id;
			return stream.str();
		}
	}

	std::string ContentRegistry::compositeKey(ContentKind kind, const std::string &stableKey)
	{
		return std::to_string(static_cast<unsigned int>(kind)) + ':' + stableKey;
	}

	std::uint64_t ContentRegistry::compositeId(ContentKind kind, std::uint32_t runtimeId)
	{
		return (static_cast<std::uint64_t>(kind) << 32u) | runtimeId;
	}

	bool ContentRegistry::registerContent(ContentDescriptor descriptor, std::string *error)
	{
		auto fail = [&](const char *message)
		{
			if (error) { *error = message; }
			return false;
		};

		if (!validStableKey(descriptor.stableKey)) { return fail("invalid stable content key"); }
		if (descriptor.kind == ContentKind::Block && descriptor.runtimeId >= 2048u)
		{
			return fail("block runtime ID exceeds the 11-bit persisted range");
		}
		if (descriptor.kind == ContentKind::Entity && descriptor.runtimeId > 255u)
		{
			return fail("entity runtime ID exceeds the persisted byte range");
		}

		const std::string key = compositeKey(descriptor.kind, descriptor.stableKey);
		if (keyToRuntimeId.find(key) != keyToRuntimeId.end())
		{
			return fail("duplicate stable content key");
		}
		const std::uint64_t id = compositeId(descriptor.kind, descriptor.runtimeId);
		if (runtimeIdToKey.find(id) != runtimeIdToKey.end())
		{
			return fail("duplicate runtime content ID");
		}

		keyToRuntimeId.emplace(key, descriptor.runtimeId);
		runtimeIdToKey.emplace(id, descriptor.stableKey);
		descriptors.push_back(std::move(descriptor));
		return true;
	}

	std::optional<std::uint32_t> ContentRegistry::resolve(ContentKind kind,
		const std::string &stableKey) const
	{
		const auto found = keyToRuntimeId.find(compositeKey(kind, stableKey));
		if (found == keyToRuntimeId.end()) { return std::nullopt; }
		return found->second;
	}

	std::optional<std::string> ContentRegistry::stableKey(ContentKind kind,
		std::uint32_t runtimeId) const
	{
		const auto found = runtimeIdToKey.find(compositeId(kind, runtimeId));
		if (found == runtimeIdToKey.end()) { return std::nullopt; }
		return found->second;
	}

	std::uint32_t ContentRegistry::resolveOrFallback(ContentKind kind,
		const std::string &key, std::uint32_t fallbackRuntimeId, bool *usedFallback) const
	{
		const auto resolved = resolve(kind, key);
		if (usedFallback) { *usedFallback = !resolved.has_value(); }
		return resolved.value_or(fallbackRuntimeId);
	}

	std::size_t ContentRegistry::size(ContentKind kind) const
	{
		return static_cast<std::size_t>(std::count_if(descriptors.begin(), descriptors.end(),
			[kind](const ContentDescriptor &descriptor) { return descriptor.kind == kind; }));
	}

	ContentRegistry createV06ContentRegistry()
	{
		ContentRegistry registry;
		for (std::uint32_t id = 0; id < V05_BLOCK_COUNT; ++id)
		{
			registry.registerContent({ContentKind::Block, id, legacyKey("block", id), true});
		}
		for (std::uint32_t id = V05_FIRST_ITEM_ID; id < V05_LAST_ITEM_EXCLUSIVE; ++id)
		{
			registry.registerContent({ContentKind::Item, id, legacyKey("item", id), true});
		}

		constexpr std::array<const char *, V05_ENTITY_TYPE_COUNT> entityKeys = {
			"mie:entity/player", "mie:entity/dropped_item", "mie:entity/zombie",
			"mie:entity/pig", "mie:entity/cat", "mie:entity/goblin",
			"mie:entity/training_dummy", "mie:entity/scarecrow",
		};
		for (std::uint32_t id = 0; id < entityKeys.size(); ++id)
		{
			registry.registerContent({ContentKind::Entity, id, entityKeys[id], true});
		}

		registry.registerContent({ContentKind::Machine, 1,
			"mie:machine/prototype_processor", false});
		registry.registerContent({ContentKind::Recipe, 1,
			"mie:recipe/prototype_processing", false});
		return registry;
	}

	ContentRegistry createV07ContentRegistry()
	{
		ContentRegistry registry = createV06ContentRegistry();

		// These IDs and stable keys were recovered from the shipped v0.7.0
		// executable. They are part of the persisted save contract now.
		constexpr std::array<const char *, 9> itemKeys = {
			"mie:item/field_guide",
			"mie:item/charcoal",
			"mie:item/tin_concentrate",
			"mie:item/tin_ingot",
			"mie:item/bronze_ingot",
			"mie:item/bronze_pickaxe",
			"mie:item/bronze_axe",
			"mie:item/bronze_shovel",
			"mie:item/bronze_sword",
		};
		for (std::uint32_t offset = 0; offset < itemKeys.size(); ++offset)
		{
			registry.registerContent({ContentKind::Item, V07_FIRST_ITEM_ID + offset,
				itemKeys[offset], false});
		}

		constexpr std::array<const char *, 7> recipeKeys = {
			"mie:recipe/process_copper_ore",
			"mie:recipe/process_lead_ore",
			"mie:recipe/process_iron_ore",
			"mie:recipe/process_silver_ore",
			"mie:recipe/process_gold_ore",
			"mie:recipe/refine_tin",
			"mie:recipe/alloy_bronze",
		};
		for (std::uint32_t offset = 0; offset < recipeKeys.size(); ++offset)
		{
			registry.registerContent({ContentKind::Recipe, 2u + offset,
				recipeKeys[offset], false});
		}

		return registry;
	}
}
