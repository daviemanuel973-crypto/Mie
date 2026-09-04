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

	// v0.9 used discovery format 1 with a 45-byte payload. A v0.10 reader must
	// preserve those bits and initialize only the newly appended item bits to 0.
	auto legacyPayload = payload;
	legacyPayload[4] = 1;
	legacyPayload[5] = static_cast<unsigned char>(RecipeDiscovery::LegacyV09StorageBytes);
	legacyPayload[6] = 0;
	legacyPayload.resize(RecipeDiscovery::HeaderBytes +
		RecipeDiscovery::LegacyV09StorageBytes);
	RecipeDiscovery migrated;
	check(migrated.readFromData(legacyPayload.data(), legacyPayload.size()) ==
		static_cast<int>(legacyPayload.size()), "a v0.9 discovery payload migrates");
	check(migrated.knowsType(5) &&
		migrated.knowsType(RecipeDiscovery::FirstItemType),
		"v0.9 learned material bits survive migration");
	check(!migrated.knowsType(RecipeDiscovery::LastItemTypeExclusive - 5),
		"v0.10 materials are not learned accidentally during migration");

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
	badVersion[4] = 3;
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
