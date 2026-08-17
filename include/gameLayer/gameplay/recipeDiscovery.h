#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

// The persisted v0.7 content ranges are compacted into one discovery bit set.
// Blocks occupy IDs [0, 211], while items occupy [2048, 2192]. Keeping the
// range constants here avoids storing a sparse 64K type-ID table per player.
struct RecipeDiscovery
{
	static constexpr std::uint16_t BlockTypeCount = 212;
	static constexpr std::uint16_t FirstItemType = 2048;
	static constexpr std::uint16_t LastItemTypeExclusive = 2193;
	static constexpr std::size_t KnownTypeCount = BlockTypeCount +
		(LastItemTypeExclusive - FirstItemType);
	static constexpr std::size_t StorageBytes = (KnownTypeCount + 7) / 8;
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

static_assert(RecipeDiscovery::KnownTypeCount == 357,
	"v0.7 recipe discovery ranges changed without a migration");
static_assert(RecipeDiscovery::StorageBytes == 45,
	"recipe discovery should remain a compact inventory extension");
