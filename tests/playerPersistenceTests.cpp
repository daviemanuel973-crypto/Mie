#include <multyPlayer/playerPersistence.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>

namespace
{
	#define REQUIRE(condition) do { if (!(condition)) { \
		std::cerr << "Requirement failed: " #condition " at line " << __LINE__ << "\n"; \
		return 1; \
	} } while (false)

	PlayerIdentity testIdentity()
	{
		PlayerIdentity result;
		for (std::size_t i = 0; i < result.bytes.size(); ++i)
		{
			result.bytes[i] = static_cast<unsigned char>(i + 1);
		}
		return result;
	}
}

int main()
{
	PlayerSaveSnapshot original;
	original.identity = testIdentity();
	original.position = {123.5, 64.25, -999.75};
	original.life = 73;
	original.maxLife = 125;
	original.hunger = 42;
	original.maxHunger = 100;
	original.gameMode = 1;
	original.inventory = {0, 1, 2, 3, 200, 255};

	const auto encoded = formatPlayerSaveSnapshot(original);
	REQUIRE(!encoded.empty());

	PlayerSaveSnapshot decoded;
	REQUIRE(parsePlayerSaveSnapshot(encoded.data(), encoded.size(), decoded));
	REQUIRE(decoded.identity == original.identity);
	REQUIRE(decoded.identity.toString() == "0102030405060708090a0b0c0d0e0f10");
	REQUIRE(decoded.position == original.position);
	REQUIRE(decoded.life == original.life);
	REQUIRE(decoded.maxLife == original.maxLife);
	REQUIRE(decoded.hunger == original.hunger);
	REQUIRE(decoded.maxHunger == original.maxHunger);
	REQUIRE(decoded.gameMode == original.gameMode);
	REQUIRE(decoded.inventory == original.inventory);

	for (std::size_t truncatedSize = 0; truncatedSize < encoded.size(); ++truncatedSize)
	{
		PlayerSaveSnapshot truncated;
		REQUIRE(!parsePlayerSaveSnapshot(encoded.data(), truncatedSize, truncated));
	}

	auto badMagic = encoded;
	badMagic[0] ^= 0xFF;
	REQUIRE(!parsePlayerSaveSnapshot(badMagic.data(), badMagic.size(), decoded));

	auto badVersion = encoded;
	const std::uint32_t unsupportedVersion = PLAYER_SAVE_FORMAT_VERSION + 1;
	std::memcpy(badVersion.data() + 8, &unsupportedVersion, sizeof(unsupportedVersion));
	REQUIRE(!parsePlayerSaveSnapshot(badVersion.data(), badVersion.size(), decoded));

	auto invalidPosition = encoded;
	const double infinity = std::numeric_limits<double>::infinity();
	std::memcpy(invalidPosition.data() + 8 + sizeof(std::uint32_t) + PLAYER_IDENTITY_SIZE,
		&infinity, sizeof(infinity));
	REQUIRE(!parsePlayerSaveSnapshot(invalidPosition.data(), invalidPosition.size(), decoded));

	PlayerSaveSnapshot oversized = original;
	oversized.inventory.resize(MAX_PLAYER_INVENTORY_SAVE_SIZE + 1);
	REQUIRE(formatPlayerSaveSnapshot(oversized).empty());

	std::cout << "Player persistence format tests passed.\n";
	return 0;
}
