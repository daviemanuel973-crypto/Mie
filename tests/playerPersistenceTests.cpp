#include <multyPlayer/playerPersistence.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <vector>

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

	std::vector<unsigned char> makeLegacyV1(const std::vector<unsigned char> &v3)
	{
		// Common payload through gameMode is 61 bytes. v0.7 inserts one starter
		// guide byte plus two uint32 masks before the legacy inventory length.
		constexpr std::size_t guideExtensionOffset = 61;
		constexpr std::size_t guideExtensionSize = 1 + sizeof(std::uint32_t) * 2;
		std::vector<unsigned char> legacy = v3;
		const std::uint32_t legacyVersion = PLAYER_SAVE_LEGACY_FORMAT_VERSION;
		std::memcpy(legacy.data() + 8, &legacyVersion, sizeof(legacyVersion));
		legacy.erase(legacy.begin() + guideExtensionOffset,
			legacy.begin() + guideExtensionOffset + guideExtensionSize);
		return legacy;
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
	original.guideState.starterFieldGuideGranted = true;
	original.guideState.completedObjectives = 0b10111;
	original.guideState.rewardedObjectives = 0b00111;
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
	REQUIRE(decoded.guideState == original.guideState);
	REQUIRE(decoded.inventory == original.inventory);

	// The pre-v0.7 version remains readable and safely defaults the new fields.
	const auto legacy = makeLegacyV1(encoded);
	PlayerSaveSnapshot legacyDecoded;
	REQUIRE(parsePlayerSaveSnapshot(legacy.data(), legacy.size(), legacyDecoded));
	REQUIRE(legacyDecoded.identity == original.identity);
	REQUIRE(!legacyDecoded.guideState.starterFieldGuideGranted);
	REQUIRE(legacyDecoded.guideState.completedObjectives == 0);
	REQUIRE(legacyDecoded.guideState.rewardedObjectives == 0);
	REQUIRE(legacyDecoded.inventory == original.inventory);

	// Corrupt/excess guide bits are sanitized and a reward can never remain set
	// for an objective that is not completed.
	PlayerSaveSnapshot dirtyGuide = original;
	dirtyGuide.guideState.completedObjectives = 0xFFFFFFFFu;
	dirtyGuide.guideState.rewardedObjectives = 0xFFFFFFFFu;
	const auto sanitizedEncoded = formatPlayerSaveSnapshot(dirtyGuide);
	PlayerSaveSnapshot sanitizedDecoded;
	REQUIRE(parsePlayerSaveSnapshot(sanitizedEncoded.data(), sanitizedEncoded.size(), sanitizedDecoded));
	REQUIRE(sanitizedDecoded.guideState.completedObjectives == PLAYER_GUIDE_OBJECTIVE_MASK);
	REQUIRE(sanitizedDecoded.guideState.rewardedObjectives == PLAYER_GUIDE_OBJECTIVE_MASK);

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