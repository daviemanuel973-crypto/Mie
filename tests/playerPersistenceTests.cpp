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

	std::vector<unsigned char> makeV3(const std::vector<unsigned char> &v4)
	{
		// Common payload through gameMode is 61 bytes. v3 contains the 9-byte
		// guide extension but not v0.9's 25-byte home extension.
		constexpr std::size_t commonPayloadSize = 61;
		constexpr std::size_t guideExtensionSize = 1 + sizeof(std::uint32_t) * 2;
		constexpr std::size_t homeExtensionSize = 1 + sizeof(double) * 3;
		std::vector<unsigned char> result = v4;
		const std::uint32_t version = PLAYER_SAVE_GUIDE_FORMAT_VERSION;
		std::memcpy(result.data() + 8, &version, sizeof(version));
		const auto homeOffset = commonPayloadSize + guideExtensionSize;
		result.erase(result.begin() + homeOffset, result.begin() + homeOffset + homeExtensionSize);
		return result;
	}

	std::vector<unsigned char> makeLegacyV1(const std::vector<unsigned char> &v4)
	{
		constexpr std::size_t commonPayloadSize = 61;
		constexpr std::size_t guideExtensionSize = 1 + sizeof(std::uint32_t) * 2;
		constexpr std::size_t homeExtensionSize = 1 + sizeof(double) * 3;
		std::vector<unsigned char> result = v4;
		const std::uint32_t version = PLAYER_SAVE_LEGACY_FORMAT_VERSION;
		std::memcpy(result.data() + 8, &version, sizeof(version));
		result.erase(result.begin() + commonPayloadSize,
			result.begin() + commonPayloadSize + guideExtensionSize + homeExtensionSize);
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
	original.guideState.starterFieldGuideGranted = true;
	original.guideState.completedObjectives = 0b10111;
	original.guideState.rewardedObjectives = 0b00111;
	original.homeState.hasHome = true;
	original.homeState.position = {120.0, 66.0, -1002.0};
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
	REQUIRE(decoded.homeState == original.homeState);
	REQUIRE(decoded.inventory == original.inventory);

	// v0.7/v0.8 v3 saves remain readable and safely default the new v0.9 home.
	const auto v3 = makeV3(encoded);
	PlayerSaveSnapshot v3Decoded;
	REQUIRE(parsePlayerSaveSnapshot(v3.data(), v3.size(), v3Decoded));
	REQUIRE(v3Decoded.identity == original.identity);
	REQUIRE(v3Decoded.guideState == original.guideState);
	REQUIRE(!v3Decoded.homeState.hasHome);
	REQUIRE(v3Decoded.homeState.position == std::array<double, 3>{});
	REQUIRE(v3Decoded.inventory == original.inventory);

	// The pre-v0.7 version also remains readable and defaults both extensions.
	const auto legacy = makeLegacyV1(encoded);
	PlayerSaveSnapshot legacyDecoded;
	REQUIRE(parsePlayerSaveSnapshot(legacy.data(), legacy.size(), legacyDecoded));
	REQUIRE(legacyDecoded.identity == original.identity);
	REQUIRE(!legacyDecoded.guideState.starterFieldGuideGranted);
	REQUIRE(legacyDecoded.guideState.completedObjectives == 0);
	REQUIRE(legacyDecoded.guideState.rewardedObjectives == 0);
	REQUIRE(!legacyDecoded.homeState.hasHome);
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

	// Invalid home anchors are never written and malformed v4 anchors are rejected.
	PlayerSaveSnapshot invalidHome = original;
	invalidHome.homeState.position[1] = std::numeric_limits<double>::infinity();
	REQUIRE(formatPlayerSaveSnapshot(invalidHome).empty());

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
