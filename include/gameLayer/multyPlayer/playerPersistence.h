#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct PlayerServer;

constexpr std::uint32_t PLAYER_IDENTITY_PROTOCOL_VERSION = 1;
constexpr std::uint32_t PLAYER_SAVE_FORMAT_VERSION = 4;
constexpr std::uint32_t PLAYER_SAVE_GUIDE_FORMAT_VERSION = 3;
constexpr std::uint32_t PLAYER_SAVE_LEGACY_FORMAT_VERSION = 1;
constexpr std::size_t PLAYER_IDENTITY_SIZE = 16;
constexpr std::size_t MAX_PLAYER_INVENTORY_SAVE_SIZE = 1024 * 1024;
constexpr std::uint32_t PLAYER_GUIDE_OBJECTIVE_MASK = 0x1Fu;

struct PlayerIdentity
{
	std::array<unsigned char, PLAYER_IDENTITY_SIZE> bytes = {};

	bool isValid() const;
	std::string toString() const;

	bool operator==(const PlayerIdentity &other) const { return bytes == other.bytes; }
	bool operator!=(const PlayerIdentity &other) const { return !(*this == other); }
};

// Persisted v0.7 Field Guide state reconstructed from the shipped 0.7.0 save
// format. Completed/rewarded are bit sets for the five first-time objectives.
struct PlayerGuideSaveState
{
	bool starterFieldGuideGranted = false;
	std::uint32_t completedObjectives = 0;
	std::uint32_t rewardedObjectives = 0;

	void sanitize()
	{
		completedObjectives &= PLAYER_GUIDE_OBJECTIVE_MASK;
		rewardedObjectives &= completedObjectives;
	}

	bool operator==(const PlayerGuideSaveState &other) const
	{
		return starterFieldGuideGranted == other.starterFieldGuideGranted &&
			completedObjectives == other.completedObjectives &&
			rewardedObjectives == other.rewardedObjectives;
	}
};

// v0.9 home extension. The save representation intentionally stays independent
// from glm/runtime structs so the on-disk layout remains explicit and testable.
struct PlayerHomeSaveState
{
	bool hasHome = false;
	std::array<double, 3> position = {};

	bool operator==(const PlayerHomeSaveState &other) const
	{
		return hasHome == other.hasHome && position == other.position;
	}
};

// This representation is intentionally independent from the runtime PlayerServer
// layout. It keeps the on-disk format stable when entity structs gain new fields.
struct PlayerSaveSnapshot
{
	PlayerIdentity identity = {};
	std::array<double, 3> position = {};
	std::int16_t life = 100;
	std::int16_t maxLife = 100;
	std::int16_t hunger = 100;
	std::int16_t maxHunger = 100;
	std::uint8_t gameMode = 0;
	PlayerGuideSaveState guideState = {};
	PlayerHomeSaveState homeState = {};
	std::vector<unsigned char> inventory;
};

std::vector<unsigned char> formatPlayerSaveSnapshot(const PlayerSaveSnapshot &snapshot);
bool parsePlayerSaveSnapshot(const void *data, std::size_t size, PlayerSaveSnapshot &snapshot);

PlayerIdentity loadOrCreateLocalPlayerIdentity();

// worldSavePath is the existing per-world WorldSaver directory.
bool loadPlayerFromDisk(const std::string &worldSavePath,
	const PlayerIdentity &identity, PlayerServer &player);
bool savePlayerToDisk(const std::string &worldSavePath,
	const PlayerIdentity &identity, const PlayerServer &player);
