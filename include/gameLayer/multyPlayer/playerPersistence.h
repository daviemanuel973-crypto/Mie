#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct PlayerServer;

constexpr std::uint32_t PLAYER_IDENTITY_PROTOCOL_VERSION = 1;
constexpr std::uint32_t PLAYER_SAVE_FORMAT_VERSION = 1;
constexpr std::size_t PLAYER_IDENTITY_SIZE = 16;
constexpr std::size_t MAX_PLAYER_INVENTORY_SAVE_SIZE = 1024 * 1024;

struct PlayerIdentity
{
	std::array<unsigned char, PLAYER_IDENTITY_SIZE> bytes = {};

	bool isValid() const;
	std::string toString() const;

	bool operator==(const PlayerIdentity &other) const { return bytes == other.bytes; }
	bool operator!=(const PlayerIdentity &other) const { return !(*this == other); }
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
