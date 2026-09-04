#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

// The persisted v0.7 content ranges are compacted into one discovery bit set.
// Blocks occupy IDs [0, 211]. Items are append-only from 2048. v0.10 grows the
// payload by one byte; the reader accepts the v0.9 45-byte payload and zero-fills
// the new discovery bits so existing players migrate without losing progress.
struct RecipeDiscovery
{
	static constexpr std::uint16_t BlockTypeCount = 212;
	static constexpr std::uint16_t FirstItemType = 2048;
	static constexpr std::uint16_t LastItemTypeExclusive = 2199;
	static constexpr std::size_t KnownTypeCount = BlockTypeCount +
		(LastItemTypeExclusive - FirstItemType);
	static constexpr std::size_t StorageBytes = (KnownTypeCount + 7) / 8;
	static constexpr std::size_t LegacyV09StorageBytes = 45;
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

static_assert(RecipeDiscovery::KnownTypeCount == 363,
	"v0.10 recipe discovery ranges changed without a migration");
static_assert(RecipeDiscovery::StorageBytes == 46,
	"v0.10 discovery payload must grow by exactly one byte");
