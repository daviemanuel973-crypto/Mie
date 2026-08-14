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
		"the last v0.7 item type can be learned");
	check(discovery.knowsType(5) && discovery.knowsType(2048) && discovery.knowsType(2192),
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
	badVersion[4] = 2;
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
