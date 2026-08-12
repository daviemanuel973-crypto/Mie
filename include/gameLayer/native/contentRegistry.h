#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mie::native
{
	enum class ContentKind : std::uint8_t
	{
		Block,
		Item,
		Entity,
		Machine,
		Recipe,
		Structure,
		LootTable,
		Region,
	};

	struct ContentDescriptor
	{
		ContentKind kind = ContentKind::Block;
		std::uint32_t runtimeId = 0;
		std::string stableKey;
		bool legacy = false;
	};

	class ContentRegistry
	{
	public:
		bool registerContent(ContentDescriptor descriptor, std::string *error = nullptr);
		std::optional<std::uint32_t> resolve(ContentKind kind, const std::string &stableKey) const;
		std::optional<std::string> stableKey(ContentKind kind, std::uint32_t runtimeId) const;
		std::uint32_t resolveOrFallback(ContentKind kind, const std::string &stableKey,
			std::uint32_t fallbackRuntimeId, bool *usedFallback = nullptr) const;
		const std::vector<ContentDescriptor> &entries() const { return descriptors; }
		std::size_t size(ContentKind kind) const;

	private:
		static std::string compositeKey(ContentKind kind, const std::string &stableKey);
		static std::uint64_t compositeId(ContentKind kind, std::uint32_t runtimeId);

		std::vector<ContentDescriptor> descriptors;
		std::unordered_map<std::string, std::uint32_t> keyToRuntimeId;
		std::unordered_map<std::uint64_t, std::string> runtimeIdToKey;
	};

	constexpr std::uint32_t V05_BLOCK_COUNT = 212;
	constexpr std::uint32_t V05_FIRST_ITEM_ID = 2048;
	constexpr std::uint32_t V05_LAST_ITEM_EXCLUSIVE = 2184;
	constexpr std::uint32_t V05_ENTITY_TYPE_COUNT = 8;

	// The shipped v0.7.0 binary appended nine survival-progression items after
	// the frozen v0.5 item range. Keep these IDs stable so old inventories and
	// recovered worlds do not reinterpret an item after an update.
	constexpr std::uint32_t V07_FIRST_ITEM_ID = V05_LAST_ITEM_EXCLUSIVE;
	constexpr std::uint32_t V07_LAST_ITEM_EXCLUSIVE = V07_FIRST_ITEM_ID + 9;

	ContentRegistry createV06ContentRegistry();
	ContentRegistry createV07ContentRegistry();
}
