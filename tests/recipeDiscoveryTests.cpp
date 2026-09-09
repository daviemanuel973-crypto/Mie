#include <gameplay/recipeDiscovery.h>

#include <iostream>
#include <vector>

namespace
{
	int failures = 0;

	void check(bool condition, const char *message)
	{
		if (!condition)
		{
			std::cerr << "FAILED: " << message << '\n';
			++failures;
		}
	}

	void setPayloadBit(std::vector<unsigned char> &payload, std::size_t index)
	{
		payload[RecipeDiscovery::HeaderBytes + index / 8] |=
			static_cast<unsigned char>(1u << (index % 8));
	}
}

int main()
{
	RecipeDiscovery discovery;
	check(discovery.learnedTypeCount() == 0, "new players start without material spoilers");
	check(!discovery.learnType(0), "air cannot be learned");
	check(!discovery.learnType(500), "unused IDs cannot be learned");
	check(!discovery.learnType(RecipeDiscovery::LastItemTypeExclusive),
		"the exclusive upper item bound is rejected");

	check(discovery.learnType(5), "a legacy block type can be learned");
	check(!discovery.learnType(5), "learning the same type twice is idempotent");
	check(discovery.learnType(RecipeDiscovery::FirstItemType),
		"the first legacy item type can be learned");
	check(discovery.learnType(RecipeDiscovery::LastItemTypeExclusive - 1),
		"the last v0.10 item type can be learned");
	check(discovery.knowsType(5) &&
		discovery.knowsType(RecipeDiscovery::FirstItemType) &&
		discovery.knowsType(RecipeDiscovery::LastItemTypeExclusive - 1),
		"learned material types remain queryable");
	check(discovery.learnedTypeCount() == 3, "the learned material count is exact");

	std::vector<unsigned char> payload;
	check(discovery.formatIntoData(payload) == RecipeDiscovery::SerializedBytes,
		"the discovery payload has a stable serialized size");
	check(payload.size() == RecipeDiscovery::SerializedBytes,
		"serialization writes one complete versioned payload");

	RecipeDiscovery decoded;
	check(decoded.readFromData(payload.data(), payload.size()) ==
		static_cast<int>(RecipeDiscovery::SerializedBytes), "a valid payload parses exactly");
	check(decoded == discovery, "recipe discovery round-trips without losing types");

	// v0.9 used 212 block bits followed by item bits. Build an authentic old
	// payload and verify that format 3 remaps, rather than merely copies, items.
	std::vector<unsigned char> legacyPayload(RecipeDiscovery::HeaderBytes +
		RecipeDiscovery::LegacyV09StorageBytes, 0);
	legacyPayload[0] = 'M'; legacyPayload[1] = 'I';
	legacyPayload[2] = 'E'; legacyPayload[3] = 'R';
	legacyPayload[4] = 1;
	legacyPayload[5] = static_cast<unsigned char>(RecipeDiscovery::LegacyV09StorageBytes);
	setPayloadBit(legacyPayload, 5);
	setPayloadBit(legacyPayload, 212);
	RecipeDiscovery migrated;
	check(migrated.readFromData(legacyPayload.data(), legacyPayload.size()) ==
		static_cast<int>(legacyPayload.size()), "a v0.9 discovery payload migrates");
	check(migrated.knowsType(5) &&
		migrated.knowsType(RecipeDiscovery::FirstItemType),
		"v0.9 learned material bits survive migration");
	check(!migrated.knowsType(RecipeDiscovery::LastItemTypeExclusive - 5),
		"v0.10 materials are not learned accidentally during migration");

	// Early v0.10 format 2 contains all five new foods but still starts item bits
	// after block 211. Their learned state must survive the six-block insertion.
	std::vector<unsigned char> earlyV010(RecipeDiscovery::HeaderBytes +
		RecipeDiscovery::LegacyV010StorageBytes, 0);
	earlyV010[0] = 'M'; earlyV010[1] = 'I'; earlyV010[2] = 'E'; earlyV010[3] = 'R';
	earlyV010[4] = 2;
	earlyV010[5] = static_cast<unsigned char>(RecipeDiscovery::LegacyV010StorageBytes);
	setPayloadBit(earlyV010, 212 +
		(RecipeDiscovery::LastItemTypeExclusive - 1 - RecipeDiscovery::FirstItemType));
	RecipeDiscovery migratedEarlyV010;
	check(migratedEarlyV010.readFromData(earlyV010.data(), earlyV010.size()) ==
		static_cast<int>(earlyV010.size()), "an early v0.10 discovery payload migrates");
	check(migratedEarlyV010.knowsType(RecipeDiscovery::LastItemTypeExclusive - 1),
		"early v0.10 food discoveries survive the block-bit offset migration");

	for (std::size_t size = 0; size < payload.size(); ++size)
	{
		RecipeDiscovery truncated;
		check(truncated.readFromData(payload.data(), size) < 0,
			"truncated discovery payloads are rejected");
	}

	auto badMagic = payload;
	badMagic[0] ^= 0xFF;
	check(decoded.readFromData(badMagic.data(), badMagic.size()) < 0,
		"invalid discovery magic is rejected");
	auto badVersion = payload;
	badVersion[4] = 4;
	check(decoded.readFromData(badVersion.data(), badVersion.size()) < 0,
		"future discovery versions are rejected safely");
	auto badLength = payload;
	badLength[5] = 0;
	badLength[6] = 0;
	check(decoded.readFromData(badLength.data(), badLength.size()) < 0,
		"payload length mismatches are rejected");

	if (failures == 0) { std::cout << "All recipe discovery tests passed\n"; }
	return failures == 0 ? 0 : 1;
}
