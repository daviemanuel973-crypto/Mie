#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

// The persisted v0.7 content ranges are compacted into one discovery bit set.
// Blocks occupy IDs [0, 217]. Items are append-only from 2048. Format 3 remaps
// the item-bit offset used by the 212-block v0.9/early-v0.10 payloads so adding
// visible crop blocks cannot reinterpret a player's learned materials.
struct RecipeDiscovery
{
	static constexpr std::uint16_t BlockTypeCount = 218;
	static constexpr std::uint16_t FirstItemType = 2048;
	static constexpr std::uint16_t LastItemTypeExclusive = 2199;
	static constexpr std::size_t KnownTypeCount = BlockTypeCount +
		(LastItemTypeExclusive - FirstItemType);
	static constexpr std::size_t StorageBytes = (KnownTypeCount + 7) / 8;
	static constexpr std::size_t LegacyV09StorageBytes = 45;
	static constexpr std::size_t LegacyV010StorageBytes = 46;
	static constexpr std::size_t HeaderBytes = 4 + 1 + 2;
	static constexpr std::size_t SerializedBytes = 4 + 1 + 2 + StorageBytes;

	bool learnType(std::uint16_t type);
	bool knowsType(std::uint16_t type) const;
	std::size_t learnedTypeCount() const;
	void sanitize();

	std::size_t formatIntoData(std::vector<unsigned char> &data) const;
	int readFromData(const void *data, std::size_t size);

	bool operator==(const RecipeDiscovery &other) const { return knownTypes == other.knownTypes; }
	bool operator!=(const RecipeDiscovery &other) const { return !(*this == other); }

private:
	static bool typeToIndex(std::uint16_t type, std::size_t &index);
	std::array<unsigned char, StorageBytes> knownTypes = {};
};

static_assert(RecipeDiscovery::KnownTypeCount == 369,
	"v0.10 recipe discovery ranges changed without a migration");
static_assert(RecipeDiscovery::StorageBytes == 47,
	"v0.10 block discovery payload must grow by exactly one byte");
